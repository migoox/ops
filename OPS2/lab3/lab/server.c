#define _GNU_SOURCE
#include "../library/mysocklib.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#define BACKLOG 3
#define HOST_COUNT 8
#define PACKET_SIZE 128

struct connection {
    int free;
    int clientfd;
};

sig_atomic_t do_work;

void usage(char *name);
void sigint_hnd(int sigNo);

void do_server(int serverfd);
int create_fd_read_set(int serverfd, struct connection waiting_foraddr[HOST_COUNT], fd_set *read_set);

void wclient_read(struct connection *client_con);

int find_free_slot(struct connection connections[HOST_COUNT]);


void client_read();

int main(int argc, char **argv)
{
    if (argc != 2) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    int serverfd = TCP_IPv4_bind_socket(atoi(argv[1]), BACKLOG);

    if (sethandler(SIG_IGN, SIGPIPE)) {
        ERR("sethandler()");
    }

    if (sethandler(sigint_hnd, SIGPIPE)) {
        ERR("sethandler()");
    }

    do_work = 1;

    do_server(serverfd);

    if (TEMP_FAILURE_RETRY(close(serverfd))) {
        ERR("close()");
    }

    return EXIT_SUCCESS;
}

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s port\n", name);
}

void sigint_hnd(int sigNo)
{
    do_work = 0;
}

void do_server(int serverfd)
{
    struct connection waiting_foraddr[HOST_COUNT];
    for (int i = 0; i < HOST_COUNT; ++i) {
        waiting_foraddr[i].free = 1;
    }

    // ignore SIGINT if pselect is not running
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    while(do_work) {
        fd_set rfds;
        int nfds = create_fd_read_set(serverfd, waiting_foraddr, &rfds);

        if (pselect(nfds + 1, &rfds, NULL, NULL, NULL, &oldmask)) {

            // new connection is pending
            if (FD_ISSET(serverfd, &rfds)) {
                int clientfd;
                if ((clientfd = add_new_client(serverfd)) >= 0) {
                    int index;
                    if ((index = find_free_slot(waiting_foraddr)) != -1) {
                        waiting_foraddr[index].free = 0;
                        waiting_foraddr[index].clientfd = clientfd;

                        printf("[Server] New client added to the waiting room.\n");
                    }
                } else {
                    fprintf(stderr, "[Server] Client connection rejected, slots are full.\n");
                }
            }


            for (int i = 0; i < HOST_COUNT; ++i) {
                if (!waiting_foraddr[i].free) {
                    if (FD_ISSET(waiting_foraddr[i].clientfd, &rfds)) {
                        wclient_read(&waiting_foraddr[i]);
                    }
                }
            }


        }

    }

    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

void wclient_read(struct connection *client_con)
{
    char buf[PACKET_SIZE];
    if (TEMP_FAILURE_RETRY(read(client_con->clientfd, buf, PACKET_SIZE)) < 0){
        ERR("read()");
    }



    printf("[Server] %s\n", buf);
}

void client_read()
{

}

int find_free_slot(struct connection connections[HOST_COUNT])
{
    for (int i = 0; i < HOST_COUNT; ++i) {
        if (connections[i].free == 1) {
            return i;
        }
    }
    return -1;
}

int create_fd_read_set(int serverfd, struct connection waiting_foraddr[HOST_COUNT], fd_set *read_set)
{
    FD_SET(serverfd, read_set);
    int nfds = serverfd;

    for (int i = 0; i < HOST_COUNT; ++i) {
        if (!waiting_foraddr[i].free) {
            FD_SET(waiting_foraddr[i].clientfd, read_set);

            if (nfds < waiting_foraddr[i].clientfd) {
                nfds = waiting_foraddr[i].clientfd;
            }
        }
    }

    return nfds;
}

