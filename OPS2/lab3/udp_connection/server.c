#define _GNU_SOURCE
#include "../library/mysocklib.h"
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define BACKLOG 3
#define MAXBUF 576
#define MAXADDR 5

volatile sig_atomic_t do_work = 1;

// represents connection data with one client
struct connections {
	// indicates if the place for the client is free
	int free;
	int32_t chunkNo;
	struct sockaddr_in addr;
};

void usage(char *name);
void sigint_handler(int sig);
int find_index(struct sockaddr_in addr, struct connections con[MAXADDR]);
void do_server(int fd);

int main(int argc, char** argv)
{
	int fd;
	if (argc != 2) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	if (sethandler(SIG_IGN, SIGPIPE))
		ERR("Seting SIGPIPE:");
	
	if (sethandler(sigint_handler, SIGINT))
		ERR("Seting SIGINT:");

	fd = UDP_IPv4_bind_socket(atoi(argv[1]));

	do_work = 1;
	do_server(fd);

	if (TEMP_FAILURE_RETRY(close(fd)) < 0)
		ERR("close");
		
	fprintf(stderr, "Server has terminated.\n");

	return EXIT_SUCCESS;
}

int find_index(struct sockaddr_in addr, struct connections con[MAXADDR])
{
	int empty = -1, pos = -1;
	for (int i = 0; i < MAXADDR; i++) {
		if (con[i].free) {
			empty = i;
		} else if (0 == memcmp(&addr, &(con[i].addr), sizeof(struct sockaddr_in))) {
			pos = i;
			break;
		}
	}

	if (-1 == pos && -1 != empty) {
		con[empty].free = 0;
		con[empty].chunkNo = 0;
		con[empty].addr = addr;
		pos = empty;
	}

	return pos;
}

void do_server(int fd)
{
	struct sockaddr_in addr;
	struct connections con[MAXADDR];
	char buf[MAXBUF];

	socklen_t size = sizeof(addr);
	int32_t chunkNo, last;

	// initialize
	for (int i = 0; i < MAXADDR; i++)
		con[i].free = 1;

	while(do_work) {
		// get data from the client
		if (TEMP_FAILURE_RETRY(recvfrom(fd, buf, MAXBUF, 0, &addr, &size) < 0))
			ERR("read:");
		
		int index;
		// find index of the client in the con array
		if ((index = find_index(addr, con)) >= 0) {

			// get datagram number and last frame bool 
			chunkNo = ntohl(*((int32_t *)buf));
			last = ntohl(*(((int32_t *)buf) + 1));
 
			// we expect frames to be received in contiguous maneer 
			// if currently received frame number is larger than the expected -> skip
			if (chunkNo > con[index].chunkNo + 1) {
				continue;
			} else if (chunkNo == con[index].chunkNo + 1) {
				// the frame is correct

				if (last) {
					// if it's the last frame, free the connection container on "index" 
					printf("Last Part %d\n%s\n", chunkNo, buf + 2 * sizeof(int32_t));
					con[index].free = 1;
				} else {
					// if it's not the last frame, increment the chunkNo
					printf("Part %d\n%s\n", chunkNo, buf + 2 * sizeof(int32_t));
					con[index].chunkNo++;
				}
			}

			// send confirmation frame (confirmation frame is the same as the received one)
			if (TEMP_FAILURE_RETRY(sendto(fd, buf, MAXBUF, 0, &addr, size)) < 0) {
				// if the client is no longer active EPIPE will be received, 
				// the server will free the container occupied by this client
				if (EPIPE == errno)
					con[index].free = 1;
			 	else
					ERR("send:");
			}
		}
	}
}

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s port\n", name);
}

void sigint_handler(int sig)
{
	do_work = 0;
}