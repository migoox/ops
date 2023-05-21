#define _GNU_SOURCE
#include "../../mysocklib/mysocklib.h"
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#define CHUNKSIZE 500
#define NMMAX 30

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s domain port \n", name);
}

void communicate(int client_fd);

int main(int argc, char **argv)
{
    if (argc != 3) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    int client_fd = TCP_IPv4_connect_socket(argv[1], argv[2]);
    communicate(client_fd);

    if (TEMP_FAILURE_RETRY(close(client_fd)) < 0)
        ERR("close");

    return EXIT_SUCCESS;
}

void communicate(int client_fd)
{
    char file[NMMAX + 1];
    char buf[CHUNKSIZE + 1];
    memset(file, 0, NMMAX + 1);
    memset(buf, 0, CHUNKSIZE + 1);

    // read the file path
    fgets(file, NMMAX + 1, stdin);

    if (file[strlen(file) - 1] == '\n')
        file[strlen(file) - 1] = '\0';

    if (bulk_write(client_fd, (void *)file, NMMAX + 1) < 0)
        ERR("bulk_write() failed");

    if (bulk_read(client_fd, (void *)buf, CHUNKSIZE) < 0)
        ERR("bulk_read() failed");

    printf("%s", buf);
}
