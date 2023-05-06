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

void usage(char *name);
void prepare_request(char **argv, int32_t data[5]);
void print_answer(int32_t data[5]);

int main(int argc, char** argv)
{
	int fd;
	int32_t data[5];
	if (argc != 5) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	if (sethandler(SIG_IGN, SIGPIPE))
		ERR("Seting SIGPIPE:");
	fd = connect_socket(argv[1], AF_UNIX, SOCK_STREAM);
	prepare_request(argv, data);
	/* Broken PIPE is treated as critical error here */
	if (bulk_write(fd, (char *)data, sizeof(int32_t[5])) < 0)
		ERR("write:");
	if (bulk_read(fd, (char *)data, sizeof(int32_t[5])) < (int)sizeof(int32_t[5]))
		ERR("read:");
	print_answer(data);
	if (TEMP_FAILURE_RETRY(close(fd)) < 0)
		ERR("close");
	return EXIT_SUCCESS;
}

void prepare_request(char **argv, int32_t data[5])
{
	data[0] = htonl(atoi(argv[2]));
	data[1] = htonl(atoi(argv[3]));
	data[2] = htonl(0);
	data[3] = htonl((int32_t)(argv[4][0]));
	data[4] = htonl(1);
}

void print_answer(int32_t data[5])
{
	if (ntohl(data[4]))
		printf("%d %c %d = %d\n", ntohl(data[0]), (char)ntohl(data[3]), ntohl(data[1]), ntohl(data[2]));
	else
		printf("Operation impossible\n");
}


void usage(char *name)
{
	fprintf(stderr, "USAGE: %s socket operand1 operand2 operation \n", name);
}