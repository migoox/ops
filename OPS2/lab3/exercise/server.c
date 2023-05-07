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
#define MAX_CONNECTIONS 5
#define NUMBERS_COUNT 3

volatile sig_atomic_t do_work = 1;

struct connection {
    int free;
    int clientfd;
    int counter;
};

void usage(char *name);
void sigint_handler(int sig);

// finds first free slot in the connections table
int find_free_slot(struct connection connections[MAX_CONNECTIONS]);

// creates set for the pselect
int create_fd_read_set(int serverfd, struct connection connections[MAX_CONNECTIONS], fd_set *set);

void do_server(int serverfd);

// does logic of the program
void work_with_client(struct connection *client_connection, int *max_num_rcvd, int *numbers_rcvd);

int main(int argc, char **argv)
{
    if (argc != 2) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    // ignore the SIGPIPE
    if (sethandler(SIG_IGN, SIGPIPE))
        ERR("setting SIGPIPE");

    if (sethandler(sigint_handler, SIGINT)) 
        ERR("sethandler()");

    int serverfd = TCP_IPv4_bind_socket(atoi(argv[1]), BACKLOG);
    // set serverfd to NONBLOCK
    if (-1 == fcntl(serverfd, F_SETFL, O_NONBLOCK)) 
        ERR("fcntl()");
    
    do_server(serverfd);

    if (TEMP_FAILURE_RETRY(close(serverfd)) < 0) {
        ERR("close()");
    }

    printf("\n[Server] Terminated\n");

    return EXIT_SUCCESS;
}

void do_server(int serverfd)
{
    struct connection connections[MAX_CONNECTIONS];

    // initialize
    int max_num_rcvd = 0;
    int numbers_rcvd = 0;

    // initialize connections
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        connections[i].counter = 0;
        connections[i].free = 1;
    }

    // ignore SIGINT if pselect is not running
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
	sigprocmask(SIG_BLOCK, &mask, &oldmask);

    printf("[Server] Ready\n");

    while (do_work) {
        fd_set rfds;
        int nfds = create_fd_read_set(serverfd, connections, &rfds);
        // wait for the event - client in the listen queue or there is something to read from the client
        if (pselect(nfds + 1, &rfds, NULL, NULL, NULL, &oldmask) > 0) {
            
            // if there is a new client add it to the connections if there is free slot
            if (FD_ISSET(serverfd, &rfds)) {
                int clientfd;
                if ((clientfd = add_new_client(serverfd)) >= 0) {
                    int index;
                    if ((index = find_free_slot(connections)) != -1) {
                        connections[index].free = 0;
                        connections[index].clientfd = clientfd;
                        printf("[Server] Connection with the client %d has been established.\n", clientfd);
                    } 
                }
            }

            for (int i = 0; i < MAX_CONNECTIONS; ++i) {
                if (FD_ISSET(connections[i].clientfd, &rfds)) {
                    work_with_client(&connections[i], &max_num_rcvd, &numbers_rcvd);
                    
                    // sync output with the console
                    fflush(stdout);
                }
            }
        } else {
            if (EINTR == errno) {
                continue;
            } 

            ERR("pselect()");
        }
    }

    printf("\n[Server] Received %d numbers.", numbers_rcvd);

    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

void work_with_client(struct connection *client_connection, int *max_num_rcvd, int *numbers_rcvd)
{
    const ssize_t buff_size = sizeof(uint32_t); 
    uint32_t buff;

    // we don't have to check for EAGAIN, since pselect provides fds that are ready to use without blocking
    if (bulk_read(client_connection->clientfd, (char *)&buff, buff_size) < 0) 
        ERR("bulk_read()");

    uint32_t num = ntohl(buff);

    printf("[Server] Received number %d from the client %d.\n", num, client_connection->clientfd);

    client_connection->counter++;
    (*numbers_rcvd)++;

    // server sends current max number to the client 
    buff = htonl(*max_num_rcvd);
    printf("[Server] Sending current max number - %d to the client %d.\n", *max_num_rcvd, client_connection->clientfd);
    if (bulk_write(client_connection->clientfd, (char *)&buff, buff_size) < 0) 
        ERR("bulk_write()");

    // end connection with the client if they've sent NUMBERS_COUNT numbers
    if (client_connection->counter >= NUMBERS_COUNT) {
        client_connection->free = 1;
        client_connection->counter = 0;
        if (TEMP_FAILURE_RETRY(close(client_connection->clientfd)) < 0) 
            ERR("close()");                            
    }

    // update max number
    if (*max_num_rcvd < num)
        *max_num_rcvd = num; 
}


int find_free_slot(struct connection connections[MAX_CONNECTIONS])
{
    for (int i = 0; i < MAX_CONNECTIONS; ++i) {
        if (connections[i].free == 1) {
            return i;
        }
    }
    return -1; 
}

int create_fd_read_set(int serverfd, struct connection connections[MAX_CONNECTIONS], fd_set *set)
{
    // initalize the set
    FD_ZERO(set);

    // add socket of the server to the mask
    FD_SET(serverfd, set);
    int nfds = serverfd;

    // add all valid connections
    for (int i = 0; i < MAX_CONNECTIONS; ++i) {
        if (connections[i].free == 0) {
            FD_SET(connections[i].clientfd, set);

            if (connections[i].clientfd > nfds) 
                nfds = connections[i].clientfd;
        }
    }

    return nfds;
}


void usage(char *name)
{
	fprintf(stderr, "USAGE: %s port\n", name);
}

void sigint_handler(int sig)
{
    do_work = 0;
}
