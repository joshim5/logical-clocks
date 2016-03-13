#include <unistd.h>
#include <sys/select.h>
#include <errno.h>
#include <arpa/inet.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/time.h>

#include <threads.h>
#include <log.h>
#include <random.h>
#include <pthread.h>
#include <globalclock.h>

static struct machine machines[THREADS];

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
	
	if (machines[id].log == NULL) {
		perror("open");
	}

	/* Initialize queue lock */
	int err = pthread_mutex_init(&machines[id].queue_lock, NULL);
	if (err) {
		perror("pthread_mutex_init");
		exit(1);
	}

	machines[id].queue = queue_create(MAX_QUEUE);
	if (machines[id].queue == NULL) {
		perror("queue_create");
		exit(1);
	}

	return 0;
}

int init_machines(void) {
	for (int i = 0; i < THREADS; i++) {
		/* Start up server thread */
		pthread_t server_id;
		
		int err = pthread_create(&server_id, NULL, server_routine, (void*) &machines[i]);
		if (err) {
			perror("pthread_create");
			return -1;
		}

		/* Start up client thread */
		pthread_t client_id;
		
		err = pthread_create(&client_id, NULL, client_routine, (void*) &machines[i]);
		if (err) {
			perror("pthread_create");
			return -1;
		}
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
			inet_pton(AF_INET, "localhost", &servaddr.sin_addr);

			/* This may not work, since the other machine may
			 * not have started its server yet. In that case,
			 * we simply loop and try again. */
			err = connect(client->connections[counter], (struct sockaddr *) &servaddr, sizeof(servaddr));
			
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
			
			if (size > 0) {
				/* Dequeue a message */
				unsigned received = queue_dequeue(mach->queue);
				
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

			int which = random_int(1, 10);		
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
	char buffer[BUF_SIZE];

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
	setsockopt(server->master_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

	/* Set up server binding configuration */
	struct sockaddr_in servaddr; 
	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(server->port);
	inet_pton(AF_INET, "localhost", &servaddr.sin_addr);
	
	/* Bind server to port */
	int err = bind(server->master_socket, (struct sockaddr *) &servaddr, sizeof(servaddr));
	if (err) {
		printf("Server: bind: error binding to port\n");
		perror("bind");
		return NULL;
	}

	/* Start listening for incoming connections; at most, THREADS - 1 */
	listen(server->master_socket, THREADS - 1);

	printf("Server: Server %d running\n", mach->id);
	
	while (1) {		
		/* Select on sockets to see if there is either a new 
		 * connection, or a new message */
		int r = select(max_fd + 1, &readfds, NULL, NULL, NULL);

		if (r < 0 && errno != EINTR) {
			printf("Server: select: error on select\n");
			return NULL;
		}

		/* New connection */
		if (FD_ISSET(server->master_socket, &readfds)) {
			/* Accept the new connection and do bookkeeping
			 * for select */
			struct sockaddr *addr;
			int fd = accept(server->master_socket, NULL, NULL);

			if (fd < 0) {
				printf("Server: accept: error on accept\n");
				perror("accept");
				return NULL;
			}

			printf("Server: New connection\n");

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

		/* New message; loop through all the connections to check */
		for (int i = 0; i < THREADS; i++) {
			int fd = server->connections[i];
		
			if (FD_ISSET(fd, &readfds)) {
				int nread = read(fd, buffer, BUF_SIZE);
				
				/* This should not happen, since we are using
				 * select */
				if (nread == -1) {
					perror("read");
					exit(1);
				}

				/* If no bytes were received, the connection
				 * was lost */
				if (nread == 0) {
					printf("Server: Socket %d disconnected!\n", fd);
					server->connections[i] = 0;
					close(fd);
					
					break;
				}
				
				/* Must nul-terminate strings */
				buffer[nread] = '\0';
				
				/* Assuming messages are always unsigned ints */
				unsigned received = strtoul(buffer, NULL, 10);

				/* Acquire queue lock, and enqueue message */
				pthread_mutex_lock(&mach->queue_lock);
				queue_enqueue(mach->queue, received);
				pthread_mutex_unlock(&mach->queue_lock);

				break;
			}
		}
	}
}

/* Sends a message to destination containing the value of the lclock. */
int send_message(int dest, unsigned lclock) {
	char buffer[BUF_SIZE];
	sprintf(buffer, "%d", lclock);
	return write(dest, buffer, strlen(buffer) + 1);
}

/* Handles an internal event. */
void internal_event(void) {
	usleep(1000);	
}
