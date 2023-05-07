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
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

#define MAXBUF 576
volatile sig_atomic_t last_signal = 0;

void sigalrm_handler(int sig);
void usage(char *name);
void send_and_confirm(int fd, struct sockaddr_in addr, char *buf1, char *buf2, ssize_t size);
void do_client(int fd, struct sockaddr_in addr, int file);

int main(int argc, char **argv)
{
	int fd, file;

	struct sockaddr_in addr;

	if (argc != 4) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	if (sethandler(SIG_IGN, SIGPIPE))
		ERR("Seting SIGPIPE:");

	if (sethandler(sigalrm_handler, SIGALRM))
		ERR("Seting SIGALRM:");

	if ((file = TEMP_FAILURE_RETRY(open(argv[3], O_RDONLY))) < 0)
		ERR("open:");

	fd = UDP_IPv4_make_socket();
	addr = IPv4_make_address(argv[1], argv[2]);

	do_client(fd, addr, file);

	if (TEMP_FAILURE_RETRY(close(fd)) < 0)
		ERR("close");

	if (TEMP_FAILURE_RETRY(close(file)) < 0)
		ERR("close");

	return EXIT_SUCCESS;
}

void send_and_confirm(int fd, struct sockaddr_in addr, char *buf1, char *buf2, ssize_t size)
{
	if (TEMP_FAILURE_RETRY(sendto(fd, buf1, size, 0, &addr, sizeof(addr))) < 0)
		ERR("sendto:");

	// set signal sending after 0.5s
	struct itimerval ts;
	memset(&ts, 0, sizeof(struct itimerval));
	ts.it_value.tv_usec = 500000;
	setitimer(ITIMER_REAL, &ts, NULL);
	last_signal = 0;

	// recv the confirm datagram from the server
	while (recv(fd, buf2, size, 0) < 0) {
		// if interrupted - try again
		if (EINTR != errno)
			ERR("recv:");

		// if time expired, exit the function -> sending failed
		if (SIGALRM == last_signal)
			break;
	}
}


void do_client(int fd, struct sockaddr_in addr, int file)
{
	char buf[MAXBUF];
	char buf2[MAXBUF];
	int offset = 2 * sizeof(int32_t);

	int32_t chunkNo = 0;
	int32_t last = 0;
	ssize_t size;

	int counter;
	do {
		// read data to send in the current datagram
		if ((size = bulk_read(file, buf + offset, MAXBUF - offset)) < 0)
			ERR("read from file:");

		// set datagram index
		*((int32_t *)buf) = htonl(++chunkNo);

		// set last frame bool value
		if (size < MAXBUF - offset) {
			last = 1;
			memset(buf + offset + size, 0, MAXBUF - offset - size);
		}
		*(((int32_t *)buf) + 1) = htonl(last);

		// prepare buf2 for confirming frame
		memset(buf2, 0, MAXBUF);
		counter = 0;
		
		// five attempts to send and get confirmation frame
		do {
			counter++;
			send_and_confirm(fd, addr, buf, buf2, MAXBUF);
		} while (*((int32_t *)buf2) != htonl(chunkNo) && counter <= 5);

		// if after 5 tries confirmation frame wasn't received - break
		if (*((int32_t *)buf2) != htonl(chunkNo) && counter > 5) {
			printf("[CLIENT] After five attempts I didn't receive a confirmation frame\n");
			break;
		}
	} while (size == MAXBUF - offset);
}

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s domain port file \n", name);
}

void sigalrm_handler(int sig)
{
	last_signal = sig;
}