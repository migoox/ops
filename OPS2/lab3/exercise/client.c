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

#define MAX_NUM 1000
#define NUMBERS_COUNT 3

void usage(char *name);

void do_client(int clientfd);

int main(int argc, char **argv)
{
    if (argc != 3) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    int clientfd = TCP_IPv4_connect_socket(argv[1], argv[2]);
    printf("[Client] Ready\n");
    do_client(clientfd);

    if (TEMP_FAILURE_RETRY(close(clientfd))) {
        ERR("close()");
    }

    printf("\n[Client] Terminated\n");

    return EXIT_SUCCESS;
}

void do_client(int clientfd)
{
    srand(time(NULL));

    const ssize_t buff_size = sizeof(uint32_t); 
    uint32_t buff;

    for (int i = 0; i < NUMBERS_COUNT; ++i) {
        uint32_t num = rand() % MAX_NUM + 1;
        buff = htonl(num);
        printf("\n[Client] Sending number %d.", num);

        if (bulk_write(clientfd, (char*)&buff, buff_size) < 0)
            ERR("bulk_write()");

        if (bulk_read(clientfd, (char*)&buff, buff_size) < 0)
            ERR("bulk_read()");

        printf("\n[Client] Received number %d from the server.",  htonl(buff));

        if (num == htonl(buff))
            fprintf(stderr, "\n[Client] HIT!");

        printf("\n");

        // sync output with the console
        fflush(stdout);

        // if it is not last iteration -> sleep
        if (i < NUMBERS_COUNT - 1)
            bulk_nanosleep(0, 750000000);
    }
    
}

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s domain port\n", name);
}