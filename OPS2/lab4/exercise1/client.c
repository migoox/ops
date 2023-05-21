#define _GNU_SOURCE
#include "../../mysocklib/mysocklib.h"
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#define TIMEOUT 15
volatile sig_atomic_t last_signal = 0;


void sigalrm_handler(int sig)
{
    last_signal = sig;
}

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s domain port time \n", name);
}

int main(int argc, char **argv)
{
    if (argc != 4) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (sethandler(SIG_IGN, SIGPIPE))
        ERR("Setting SIGPIPE:");

    if (sethandler(sigalrm_handler, SIGALRM))
        ERR("Setting SIGALRM:");

    int client_fd = UDP_IPv4_make_socket();
    struct sockaddr_in addr = IPv4_make_address(argv[1], argv[2]);
    int16_t time = htons(atoi(argv[3]));
    const int16_t deny = -1;

    /* Broken PIPE is treated as critical error here */
    if (TEMP_FAILURE_RETRY(sendto(client_fd, (char *)&time, sizeof(int16_t), 0, &addr, sizeof(addr))) < 0)
        ERR("sendto() failed");

    alarm(TIMEOUT);

    while (recv(client_fd, (char *)&time, sizeof(int16_t), 0) < 0) {
        if (errno != EINTR)
            ERR("recv() failed");
        if (last_signal == SIGALRM)
            break;
    }

    if (last_signal == SIGALRM)
        printf("[CLIENT] Timeout\n");
    else if (time == deny)
        printf("[CLIENT] Service denied\n");
    else
        printf("[CLIENT] Time has expired\n");

    if (TEMP_FAILURE_RETRY(close(client_fd)) < 0)
        ERR("close");

    return EXIT_SUCCESS;
}