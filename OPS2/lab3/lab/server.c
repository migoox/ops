#define _GNU_SOURCE
#include "../library/mysocklib.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#define BACKLOG 3
#define HOST_COUNT 8
#define PACKET_SIZE 10

struct connection {
    int free;
    int clientfd;
    char buff[PACKET_SIZE + 1];
    int offset;
};

sig_atomic_t do_work;

void usage(char *name);
void sigint_hnd(int sigNo);

void do_server(int serverfd);
int create_fd_read_set(int serverfd, struct connection waiting_foraddr[HOST_COUNT], fd_set *read_set);

void handle_wclient(struct connection *client_con, struct connection connections[HOST_COUNT]);
void handle_client(struct connection *client_con);

int read_data(struct connection *client_con);

int find_free_slot(struct connection connections[HOST_COUNT]);

void disconnect(struct connection *con);

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

void disconnect(struct connection *con)
{
    con->free = 1;
    con->offset = 0;

    if (TEMP_FAILURE_RETRY(close(con->clientfd)) < 0) {
        ERR("close()");
    }
    memset(con->buff, 0, PACKET_SIZE);
}

void do_server(int serverfd)
{
    fprintf(stderr, "[Server] Ready\n");

    struct connection waiting_foraddr[HOST_COUNT];
    struct connection connections[HOST_COUNT];

    // initialize
    for (int i = 0; i < HOST_COUNT; ++i) {
        connections[i].free = 1;
        connections[i].offset = 0;
        waiting_foraddr[i].free = 1;
        waiting_foraddr[i].offset = 0;
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

                        if (fcntl(clientfd, F_SETFL, O_NONBLOCK) == -1) {
                            ERR("fcntl()");
                        }

                        printf("[Server] New client added to the waiting room\n");
                    }
                } else {
                    fprintf(stderr, "[Server] Client connection rejected, slots are full.\n");
                }
            }

            // handle waiting clients
            for (int i = 0; i < HOST_COUNT; ++i) {
                handle_wclient(&waiting_foraddr[i], connections);
            }

            // handle active clients
            for (int i = 0; i < HOST_COUNT; ++i) {
                handle_client(&connections[i]);
            }
        }
    }

    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

void handle_wclient(struct connection *client_con, struct connection connections[HOST_COUNT])
{
    if (client_con->free) {
        return;
    }

    // read data
    int size = TEMP_FAILURE_RETRY(read(client_con->clientfd,client_con->buff + client_con->offset,
                                       PACKET_SIZE - client_con->offset));
    // throw error
    if (size < 0) {
        if (errno == EAGAIN) {
            return;
        } else {
            ERR("read()");
        }
    }

    // handle eof
    if (size == 0) {
        disconnect(client_con);
        return;
    }

    // update offset
    client_con->offset += size;

    // find end
    char* end = strchr(client_con->buff, '$');

    if (end == NULL) {
        if (client_con->offset == PACKET_SIZE) {
            fprintf(stderr, "[Server] Waiting client -  incorrect request.\n");

            // clear buffer
            client_con->offset = 0;
        }
        return;
    }

    *end = '\0';
    int32_t req_addr = atoi(client_con->buff);

    fprintf(stderr, "[Server] Waiting client requested adress %d\n", req_addr);

    if (req_addr < HOST_COUNT && req_addr > 0 && connections[req_addr].free) {
        // make waiting client a valid client
        connections[req_addr].free = 0;
        connections[req_addr].offset = 0;
        connections[req_addr].clientfd = client_con->clientfd;
        memset(connections[req_addr].buff, 0, PACKET_SIZE);

        client_con->free = 1;
        fprintf(stderr, "[Server] Adress %d is valid.\n", req_addr);
    } else {
        fprintf(stderr, "[Server] Wrong address: adress %d is occupied or invalid.\n", req_addr);
    }

    // clear buffer
    client_con->offset = 0;
}

void handle_client(struct connection *client_con)
{
    if (client_con->free) {
        return;
    }

    // read data
    int size = TEMP_FAILURE_RETRY(read(client_con->clientfd,client_con->buff + client_con->offset,
                                       PACKET_SIZE - client_con->offset));
    // throw error
    if (size < 0) {
        if (errno == EAGAIN) {
            return;
        } else {
            ERR("read()");
        }
    }

    if (size == 0) {
        disconnect(client_con);
        return;
    }

    // update offset
    client_con->offset += size;

    // find end
    //char* end = strchr(client_con->buff, '$');

    fprintf(stderr, "[Server] Received: %c\n", client_con->buff[0]);
    // client_con->offset = 0;
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

