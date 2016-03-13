#include <stdio.h>

#define RECV_MSG_LOG 0
#define SEND_MSG_LOG 1
#define INTR_EVENT_LOG 2

#define LOGFILENAME_MAX 20
#define LOGBUF_SIZE 1024

void write_to_log(int type, unsigned message, unsigned lclock,
		  unsigned queue_size, FILE *log);
