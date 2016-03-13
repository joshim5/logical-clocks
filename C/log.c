#include <log.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <globalclock.h>

/* Very ugly function, but it does the job. That's what I get for not
 * using Python and hating I/O in low-level... */
void write_to_log(int type, unsigned message, unsigned lclock, 
		  unsigned queue_size, FILE *log) 
{	
	char buffer[LOGBUF_SIZE];

	switch(type) {
		case RECV_MSG_LOG:
			fprintf(log,
				"==============\n"
				"Received Message: %u\n"
				"Logical Clock After Receive: %u\n"
				"Queue Size Before Receive: %u\n"
				"Global Time: %ld\n"
				"==============\n",
				message, lclock, queue_size, time(NULL));
			
			break;
		case SEND_MSG_LOG: 
			fprintf(log,
				"==============\n"
				"Sent Message: %u\n"
				"Logical Clock After Sent: %u\n"
				"Global Time: %ld\n"
				"==============\n",
				message, lclock, time(NULL));

			break;
		case INTR_EVENT_LOG:
			fprintf(log,
				"==============\n"
				"Internal Event\n"
				"Logical Clock After Event: %u\n"
				"Global Time: %ld\n"
				"==============\n",
				lclock, time(NULL));	
			break;
		default:
			printf("write_to_log: Unknown type\n");
	}

	fflush(log);
}