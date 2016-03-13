# logical-cloks in C

This is the lab notebook for the C implementation of the logical-clocks
assignment.

## Preliminary Discussions

### Machines

Before implementing, we should decide:

* How are the machines going to be implemented? Will each one be a forked
process, a thread, or a completely separate process altogether?

* Given that machines are implemented as above, how will the server/client
portions of each machine be implemented? Each machine has to receive messages
from others and be able to send messages to them too.

* How will initialization take place? We have to guarantee that all machines
are connected to each other.

Our initial idea was to implement a separate process altogether for each
machine. Then, we could easily control the initialization phase manually,
by first using a command-line interface to start up the servers and then,
once that is done, start sending messages. In this scenario, we would
have a thread for receiving messages (denoted as the `server`) and another
one to send messages and handle internal events (denoted as the `client`).

However, after starting this implementation, we decided to change this approach
to one single application (otherwise, it would be quite annoying to start
three different processes manually and set them to work). Then, we decided
to spawn two threads for each machine, one for the `client` and one for the
`server`. A machine will have a global state, given by `struct machine`.

Our `server` works by using `select`, rather than spawning a thread or process
(ew) for each socket connection.

### Dependencies

We are going to need:

* Some sort of randomness generator;
* Some kind of queue, protected by a lock;
* Threading library;
* Time handling;
* Socket handling.

Since we are dealing with C, we initally chose to use the simple `rand()` from
`<stdlib.h>`; a simple circular queue provided by our own `<queue.h>`; the
mutex and threads from `<pthread.h>`; `gettimeofday` and `time` from 
`<sys/time.h>` and `<time.h>`; and sockets as given by `<sys/sockets.h>`.

Possibly we will want to use something better than `rand()` for randomness,
since it can be pretty obvious and repeating, especially if using modulo
arithmetic.

For time management, we will need a way to find how many `usecs` to `usleep`
until 1 second has passed. This may be a bit... boring.

## Machine Data Structure

We decided to represent machines as a global array of `struct machine`. 
This struct is as follows:

```C
struct client {
	int ticks;
	int connections[THREADS - 1];
}

struct server {
	int port;
	int master_socket;
	int connections[THREADS - 1];
}

struct machine {
	int id;
	unsigned lclock;
	FILE *log;

	pthread_mutex_t queue_lock;
	struct queue *queue;
	struct server server;
	struct client client;
}
```

The client can thus only run `ticks` instructions per second, and has
`THREADS - 1` connections. The server is running on `port`, with the main
socket `master_socket` and with `THREADS - 1` connections. The machine
also has an `id` and a logical clock `lclock` (which is only updated and
used by the `client` and thus does not need synchronization). The `queue`
will need to be protected by a mutex.

## Implementation Progress

This section was written as we were coding and therefore may not be very
organized, but it reflects what we were doing.

## Random

**Saturday, March 13th, noon**

Implemented `random_int` in `<random.h>`. This is using `rand()` and I do not
like it; it keeps repeating patterns. Later, implement Mersenne Twister
or something. Sometimes I don't like C...

## Time

**Saturday, March 13th, 12:30pm**

Implemented `cmptime`, a function that takes two `struct timeval` and computes
the difference into a third one. Returns 1 if this difference is > 1 (in
which case the third `timeval` is 0). Returns 0 if the differente is < 1.

## Queue

**Saturday, March 13th, 1pm**

Implemented this fairly quickly. Seems to work. May want to check later!

**Saturday, March 13th, 9:45pm**

Fixed the queue. I was adding to its rear before enqueuing. So there was always
a 0 in the beginning (since I `memset` every element to 0). Switching some
things around fixes it.

## Server

**Saturday, March 13th, 2:15pm**

Server basic implementation is done. It's echoing stuff from a very simple
client..

**Saturday, March 13th, 3pm**

Server does not seem to work. For some reason, even though I am selecting
on all file descriptors, I keep getting `EAGAIN` on `errno` at least 3 times
per run. Sometimes extra `0`'s are being dequeued, when the queue's size
is 0 (which makes me think maybe the queue has an issue).

**Saturday, March 13th, 4:45pm**

Fixed bug with `EAGAIN`. I was forcing a `read()` on sockets that were not
ready to be read, but I thought they were because of select. But I was missing
a `continue` by the end of the code where we actually add the fds to `readfds`,
which is what `select` checks. Since I was adding it to the `readfds` but not
checking the select, they could be not ready yet. Adding the `continue` makes
the select run again and fixes it.

**Saturday, March 13th, 10:30pm**

Found another bug. I was not appropriately handling errors on `read` in the
server. Oops.

## Client

**Saturday, March 13th, 2pm**

Client is able to connect to servers. But I still have to figure out a way
to make everyone connect to everyone before sending messages.

**Saturday, March 13th, 4pm**

Okay, so finished doing a very simple implementation of how to wait
for everybody. Basically instead of returning an error and exiting on
`connect`, simply keep trying on a `while` loop until it works. It is not
particularly beautiful and I should probably add some kind of limit on
trials later.

**Saturday, March 13th, 4:30pm**

Added logical clock logic to clients; before, they were simply sending
random numbers around.

**Saturday, March 13th, 8pm**

Came back and implemented the `write_to_log` function. Now I can see the
logs on file, rather than printing it to console. However, it seems to be
writing binary files rather than text files. I can read it on `vim` but
not on `Sublime`. I hate `C` and `<stdio.h>`. Should have just called `write`
directly...

**Saturday, March 13th, 8:15pm**

Ah ha! Nevermind, I was using `fwrite`. Switching to `fprintf` made the code
look better (no need for `snprintf` anymore) and made it work. But I still
get way too many 0s in the beginning. Probably something on the queue.

**Sunday, March 13th, 1am**

Decided to do the timing in the client. Had not implemented that yet. Simply
added `cmptime` functionality in the client. Not much changed.

## General Things

**Sunday, March 13th, 1:30am**

Refactored code. A bunch of stuff was very ugly and several functions were
clobbed together in `threads.c`. Separated stuff into nice files. Oh,
and commented the code. (Ops).

**Sunday, March 13th, 6am**

Now code will clean up after itself, closing file descriptors, sockets and
everything. Also, it will cancel the threads after a timeout. Currently
the timeout is set to 300 seconds.

# Benchmarking

To be done...
