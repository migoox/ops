#define _GNU_SOURCE
#include "../../mysocklib/mysocklib.h"
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

#define BUFF_SIZE 128
#define CMD_BUFF_SIZE 8

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s port\n", name);
}

void server_recv(int serverfd, int internal_key, int internal_prob)
{
    struct sockaddr_in client_addr;
    socklen_t len;
    char buff[BUFF_SIZE];
    if (TEMP_FAILURE_RETRY(recvfrom(serverfd, buff, BUFF_SIZE, 0,
                                 &client_addr, &len)) < 0) {
     ERR("recvfrom");
    }

    // get the generated number
    uint32_t num = ntohl((((uint32_t *) buff)[0]));

    fprintf(stderr, "[Server] Received number: %d\n", num);

    int16_t rand_num = (rand() % 100) + 1;

    if (rand_num > internal_prob) {
        return;
    }

    fprintf(stderr, "[Server] Sending confirmation frame\n");
    memset(buff, 0, BUFF_SIZE);
    ((uint32_t *)buff)[0] = htonl(htonl(num + internal_key));
    if (TEMP_FAILURE_RETRY(sendto(serverfd, buff, BUFF_SIZE, 0, &client_addr, len)) < 0) {
        if (EPIPE != errno)
            ERR("sendto");
    }
}

void handle_cmd(int serverfd, int internal_key, int *internal_prob, int *do_work)
{
    char buff[CMD_BUFF_SIZE];
    char format[20];
    sprintf(format, "\%%%ds", CMD_BUFF_SIZE - 1);
    scanf(format, buff);

    if (buff[0] == 'U') {
        // increase probability
        *internal_prob = ((*internal_prob > 90) ? 100 : (*internal_prob + 10));
    } else if (buff[0] == 'D') {
        // decrease probability
        *internal_prob = ((*internal_prob < 10) ? 0 : (*internal_prob - 10));
    } else if (buff[0] == 'P') {
        // prints the internal prob and key
        printf("[Server] Key=%d, Probability=%d%%\n", internal_key, *internal_prob);
    } else if (buff[0] == 'E') {
        *do_work = 0;
    } else {
        printf("UKNOWN COMMAND\n");
    }
}

void do_server(int serverfd, int internal_key, int internal_prob)
{
    fd_set base_rfds;
    // initalize the set
    FD_ZERO(&base_rfds);

    // add socket of the server to the mask
    FD_SET(serverfd, &base_rfds);   // server file descriptor
    FD_SET(0, &base_rfds);          // stdin file descriptor

    int do_work = 1;
    while (do_work) {
        fd_set rfds = base_rfds;
        if (select(serverfd + 1, &rfds, NULL, NULL, NULL) > 0) {
             if (FD_ISSET(0, &rfds)) {
                handle_cmd(serverfd, internal_key, &internal_prob, &do_work);
             }
             if (FD_ISSET(serverfd, &rfds)) {
                 server_recv(serverfd, internal_key, internal_prob);
             }
        } else {
            if (EINTR == errno)
                continue;

            ERR("pselect");
        }
    }
}

int main(int argc, char** argv)
{
    if (argc != 3) {
        usage(argv[0]);
    }

    srand(time(NULL));

    if (sethandler(SIG_IGN, SIGPIPE) < 0) {
        ERR("sethandler");
    }

    // server can be closed by 'E' command
    if (sethandler(SIG_IGN, SIGINT) < 0) {
        ERR("sethandler");
    }

    int32_t internal_key = atoi(argv[1]);
    int16_t internal_prob = atoi(argv[2]);
    int serverfd = UDP_IPv4_bind_socket(atoi(argv[3]));

    fprintf(stderr, "[Server] Started\n");

    do_server(serverfd, internal_key, internal_prob);

    if (TEMP_FAILURE_RETRY(close(serverfd)) < 0) {
        ERR("close");
    }
    fprintf(stderr, "[Server] Closed\n");

    return EXIT_SUCCESS;
}