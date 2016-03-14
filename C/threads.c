#include <unistd.h>
#include <sys/select.h>
#include <errno.h>
#include <arpa/inet.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/time.h>
#include <assert.h>

#include <threads.h>
#include <log.h>
#include <random.h>
#include <pthread.h>
#include <globalclock.h>

struct machine machines[THREADS];

/* Initialize a machine with a certain number of ticks and an id. */
int pre_init_machine(int ticks, int id) {
	machines[id].client.ticks = ticks;
	machines[id].id = id;

	/* Hopefully these ports are not being used */
	machines[id].server.port = 6666 + id;

	/* Open a log file */
	char filename[LOGFILENAME_MAX];
	sprintf(filename, "log/machine%d.log", id);
	machines[id].log = fopen(filename, "w+");
	
	fprintf(machines[id].log, "Number of Ticks: %d\n\n", ticks);

	if (machines[id].log == NULL) {
		perror("open");
	}

	/* Initialize queue lock */
	int err = pthread_mutex_init(&machines[id].queue_lock, NULL);
	if (err) {
		perror("pthread_mutex_init");
		exit(1);
	}

	err = pthread_cond_init(&machines[id].queue_cv, NULL);
	if (err) {
		perror("pthread_cond_init");
		exit(1);
	}

	machines[id].queue = queue_create(MAX_QUEUE);
	if (machines[id].queue == NULL) {
		perror("queue_create");
		exit(1);
	}

	return 0;
}

/* Client thread entrypoint. The argument must be a pointer to the 
 * corresponding machine struct. */
void *client_routine(void *arg) {
	struct machine *mach = (struct machine *) arg;
	struct client *client = &mach->client;

	/* Connects to all other machines */
	for (int i = 0, counter = 0; i < THREADS; i++) {
		/* Does not connect to itself */
		if (i == mach->id)
			continue;
		
		struct server *server = &machines[i].server;
		client->connections[counter] = socket(AF_INET, SOCK_STREAM, 0);

		int err = -1;
		
		while (err != 0) {
			/* Sets up socket to try connection */
			struct sockaddr_in servaddr;
			bzero(&servaddr, sizeof(servaddr));

			servaddr.sin_family = AF_INET;
			servaddr.sin_port = htons(server->port);
			inet_pton(AF_INET, "localhost", &servaddr); 

			/* This may not work, since the other machine may
			 * not have started its server yet. In that case,
			 * we simply loop and try again. */
			err = connect(client->connections[counter], (struct sockaddr *) &servaddr, 
					sizeof(servaddr));
			
			if (err != 0) { 
				perror("connect");
				continue;
			}

			/* Successful connection */
			printf("Client %d: connected\n", mach->id);
		}

		counter++;
	}		

	/* If we got here, everything is connected. Then we only need to
	 * start sending messages. */

	while (1) {
		/* Keep track of start of this set of clock cycles */
		struct timeval tv_start;
		gettimeofday(&tv_start, NULL);
		
		struct timeval tv_end;
		struct timeval tv_remains;
		
		for (int i = 0; i < client->ticks; i++) {
			/* Checks if there is a new message. We require
			 * the lock just to guarantee that a new
			 * message will not arrive when we are fetching it. */
			
			pthread_mutex_lock(&mach->queue_lock);
		
			unsigned size = queue_size(mach->queue);
			unsigned capacity = queue_capacity(mach->queue);
			
			if (size == capacity)
				pthread_cond_broadcast(&mach->queue_cv);
			
			if (size > 0) {
				/* Dequeue a message */
				unsigned received = queue_dequeue(mach->queue);
				
				/* Notice that size > 0 and we have a lock;
				 * so this should not fail */
				if (received == -1) {
					perror("queue_dequeue");
					exit(1);
				}

				/* Logical clock logic for receiving message */
				unsigned max = mach->lclock;
				if (received > mach->lclock) 
					max = received;

				mach->lclock = max + 1;
				
				/* Write appropriate log */
				write_to_log(RECV_MSG_LOG, received, mach->lclock, size, mach->log);
			
				pthread_mutex_unlock(&mach->queue_lock);

				continue;
			}

			pthread_mutex_unlock(&mach->queue_lock);
		
			/* Otherwise, we pick a random number, and act 
			 * according to the specification */

			int which = random_int(1, 5);		
			unsigned message = mach->lclock;	

			switch(which) {
				case 1:
					/* Send message to one machine */
					send_message(client->connections[0], message);
					mach->lclock++;
				
					/* Write appropriate log */
					write_to_log(SEND_MSG_LOG, message, mach->lclock, 0, mach->log); 

					break;
				case 2:
					/* Send message to other machine */
					send_message(client->connections[1], message);
					mach->lclock++;
					
					/* Write appropriate log */
					write_to_log(SEND_MSG_LOG, message, mach->lclock, 0, mach->log); 
					break;
				case 3:
					/* Send message to both machines */
					send_message(client->connections[0], message);
					mach->lclock++;
					i++;
					
					/* Write log for first send */
					write_to_log(SEND_MSG_LOG, message, mach->lclock, 0, mach->log); 
					
					/* If the second send would do more instructions
					 * than allowed per second, stop */
					if (i == client->ticks)
						break;
					
					message = mach->lclock;
					send_message(client->connections[1], message);

					mach->lclock++;
					
					/* Write log for second send */
					write_to_log(SEND_MSG_LOG, message, mach->lclock, 0, mach->log); 
					
					break;
				default:
					internal_event();
					
					mach->lclock++;
					
					/* Write appropriate log message */
					write_to_log(INTR_EVENT_LOG, 0, mach->lclock, 0, mach->log);
					break;
			}

			/* This is highly unlikely, but if a second has passed
			 * since the beginning of this loop over the ticks, then we
			 * stop counting how many operations we did this
			 * second to start count again. */
			gettimeofday(&tv_end, NULL);
			if (cmptime(&tv_start, &tv_end, &tv_remains) == 1)
				break;
		}

		/* In case we finished the clock cycle before a "real" second,
		 * then we wait for the remaining time; this guarantees no
		 * more messages will be sent during this second. */
		gettimeofday(&tv_end, NULL);
		
		if (cmptime(&tv_start, &tv_end, &tv_remains) == 0) 
			usleep(1000000 - tv_remains.tv_usec);
	}
}

/* Server thread entry point. The argument must be a pointer to
 * the corresponding machine's struct. */
void *server_routine(void *arg) {
	struct machine *mach = (struct machine *) arg;
	struct server *server = &mach->server;

	/* Set of file descriptors to select on */
	fd_set readfds;
	FD_ZERO(&readfds);
	
	/* Create server socket */
	server->master_socket = socket(AF_INET, SOCK_STREAM, 0);
	FD_SET(server->master_socket, &readfds);

	/* max_fd is annoying; select requires to know what the greatest
	 * fd in the set is. So we keep track of this. */
	int max_fd = server->master_socket;

	/* Set the server socket to non-blocking, to handle multiple
	 * connections. Also enable reuse of addresses */
	fcntl(server->master_socket, F_SETFL, O_NONBLOCK);

	int yes = 1;
	setsockopt(server->master_socket, SOL_SOCKET, SO_REUSEADDR, &yes, 
		sizeof(int));

	/* Set up server binding configuration */
	struct sockaddr_in servaddr; 
	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(server->port);
	servaddr.sin_addr.s_addr = INADDR_ANY;

	/* Bind server to port */
	int err = bind(server->master_socket, (struct sockaddr *) &servaddr, 
		sizeof(servaddr));
	if (err) {
		printf("Server: bind: error binding to port\n");
		perror("bind");
		return NULL;
	}

	/* Start listening for incoming connections; at most, THREADS - 1 */
	listen(server->master_socket, THREADS - 1);

	printf("Server: Server %d running\n", mach->id);
	
	while (1) {
		struct timeval tv;
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		/* Select on sockets to see if there is either a new 
		 connection, or a new message. Times out for half a second.
		 This is just to let all the connections happen. */ 
		int r = select(max_fd + 1, &readfds, NULL, NULL, &tv);

		if (r < 0 && errno != EINTR) {
			printf("Server: select: error on select\n");
			return NULL;
		}

		/* New message; loop through all the connections to check */
		err = receive_messages(mach, &readfds);
		if (err) {
			perror("receive_messages");
			exit(1);
		}

		/* Check for new connections */
		if (FD_ISSET(server->master_socket, &readfds)) {
			/* Accept the new connection and do bookkeeping
			 * for select */
			struct sockaddr *addr;
			int fd = accept(server->master_socket, NULL, NULL);

			if (fd < 0) {
				perror("accept");
				return NULL;
			}

			printf("Server: New connection %d\n", fd);

			for (int i = 0; i < THREADS; i++) {
				if (server->connections[i] == 0) {
					server->connections[i] = fd;
					FD_SET(fd, &readfds);
					
					if (fd > max_fd)
						max_fd = fd;
					
					break;
				}
			}

			continue;
		}
	}
}

/* Receives all messages for a given machine. Keeps looping. */
int receive_messages(struct machine *mach, fd_set *readfds) {
	struct server *server = &mach->server;
	
	for (int i = 0; i < THREADS - 1; i++) {
		int fd = server->connections[i];
	
		if (FD_ISSET(fd, readfds)) {
			struct server *server = &mach->server;
			unsigned bytes;
			char buf[BUF_SIZE];
		
			/* Check if there is anything really to read in
			 * this file descriptor. */
			bytes = recv(fd, &buf, BUF_SIZE, MSG_PEEK);
			
			/* Loop while there are bytes to read */
			while (bytes > 0) {
				uint32_t buffer = 0;
				unsigned curlen = 0;
				unsigned length = sizeof(uint32_t);

				/* Assumes we first get 4 bytes defining
				 * the length of the rest of the message.
				 * This will always be 4 bytes in this case. */
				while (curlen < length) {
					int nread = recv(fd, ((char *) &buffer) + curlen, length - curlen, 0);
					if (nread == -1) {
						if (errno == EAGAIN)
							continue;

						perror("recv");
						return -1;
					}

					if (nread == 0) {
						printf("Server: Socket %d disconnected\n", fd);
						server->connections[i] = 0;
						close(fd);

						break;
					}

					curlen += nread;
				}

				/* The length is the buffer read, so now
				 * we fetch that amount of bytes. */
				length = htonl(buffer);

				curlen = 0;
				while (curlen < length) {
					int nread = recv(fd, ((char *) &buffer) + curlen, length - curlen, 0);
					if (nread == -1) {
						if (errno == EAGAIN)
							continue;

						perror("recv");
						return -1;
					}

					if (nread == 0) {
						printf("Server: Socket %d disconnected\n", fd);
						server->connections[i] = 0;
						close(fd);

						break;
					}

					curlen += nread;
				}
					
				/* The buffer here is simply a uint32_t again */
				unsigned received = ntohl(buffer); 

				/* Acquire queue lock, and enqueue message */
				pthread_mutex_lock(&mach->queue_lock);

				unsigned size = queue_size(mach->queue);
				unsigned capacity = queue_capacity(mach->queue);

				while (size >= capacity)
					pthread_cond_wait(&mach->queue_cv, &mach->queue_lock);

				/* We waited on a condition variable that
				 * should guarantee that the queue is not full.
				 * So, this should not fail. */
				int err = queue_enqueue(mach->queue, received);
				if (err) {
					perror("queue_enqueue");
					return -1;
				}

				pthread_mutex_unlock(&mach->queue_lock);

				/* Check again if there are new messages
				 * in this fd */
				bytes = recv(fd, &buf, 1, MSG_PEEK);
			} 
		}
	}

	return 0;
}

/* Sends a message to destination containing the value of the lclock. */
int send_message(int dest, unsigned lclock) {
	uint32_t message[2];

	message[0] = htonl(4);
	message[1] = htonl(lclock);

	int nwritten = -1;

	fd_set writefds;
	FD_ZERO(&writefds);
	FD_SET(dest, &writefds);

	int r = select(dest + 1, NULL, &writefds, NULL, NULL);
	if (r == -1) {
		perror("select");
		return -1;
	}

	nwritten = send(dest, message, 2 * sizeof(uint32_t), 0);

	/* We are selecting so this should not happen */
	if (nwritten == -1) {
		perror("send_message");
		return -1;
	}

	return 0;
}

/* Handles an internal event. */
void internal_event(void) {
}
