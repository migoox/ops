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

#define QUESTION_MAX_SIZE 512
#define ANSWER_MAX_SIZE 64
#define MAX_QUESTIONS 128

#define BACKLOG 3

volatile sig_atomic_t last_signal;
volatile sig_atomic_t do_work;

struct connection {
    int free;
    int clientfd;
    int question_id;
    char offset;
    int msg_sent;
};

struct quiz {
    char questions[MAX_QUESTIONS][QUESTION_MAX_SIZE];
    char answers[MAX_QUESTIONS][ANSWER_MAX_SIZE];
    size_t size;
};

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s port max_clients file_path\n", name);
}

void sigusr1_handler(int sigNo)
{
    last_signal = SIGUSR1;
}

void sigint_handler(int sigNo)
{
    do_work = 0;
}

void read_data(char* path, struct quiz *quiz_data);

int find_free(struct connection *connections, int max_clients);

void do_server(int serverfd, int max_clients, struct quiz *quiz_data);

int main(int argc, char **argv)
{
    if (argc != 4) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    srand(time(NULL));

    if (sethandler(SIG_IGN, SIGPIPE ) < 0) {
        ERR("sethandler");
    }
    if (sethandler(sigint_handler, SIGINT) < 0) {
        ERR("sethandler");
    }
    if (sethandler(sigusr1_handler, SIGUSR1) < 0) {
        ERR("sethandler");
    }

    fprintf(stderr, "[Server] Reading quiz data...\n");
    struct quiz quiz_data;
    read_data(argv[3], &quiz_data);

    int serverfd = TCP_IPv4_bind_socket(atoi(argv[1]), BACKLOG);
    // set nonblock
    int new_flags = fcntl(serverfd, F_GETFL) | O_NONBLOCK;
    fcntl(serverfd, F_SETFL, new_flags);


    fprintf(stderr, "[Server] Started\n");
    do_work = 1;
    do_server(serverfd, atoi(argv[2]), &quiz_data);

    if (TEMP_FAILURE_RETRY(close(serverfd)) < 0) {
        ERR("close");
    }

    fprintf(stderr, "[Server] Closed");

    return EXIT_SUCCESS;
}

void read_data(char* path, struct quiz *quiz_data)
{
    FILE *file;
    if ((file = fopen(path, "r")) == NULL) {
        ERR("fopen");
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t size;
    // If  *lineptr is set to NULL and *n is set 0 before the call, then getline() will
    // allocate a buffer for storing the line. (man getline)
    int i = 0;
    while ((size = getline(&line, &len, file)) != -1) {
        strcpy(quiz_data->questions[i], line);
        // get the answer
        if ((size = getline(&line, &len, file)) == -1) {
            break;
        }
        strcpy(quiz_data->answers[i], line);
        // get blank line
        if ((size = getline(&line, &len, file)) == -1) {
            break;
        }
        ++i;
    }
    fclose(file);
    quiz_data->size = i;

    if (line) {
        free(line);
    }
}

int find_free(struct connection *connections, int max_clients)
{
    for (int i = 0; i < max_clients; ++i) {
        if (connections[i].free) {
            return i;
        }
    }

    return -1;
}
void new_client_event(int serverfd, int max_clients, struct quiz *quiz_data,
        struct connection *connections)
{
    int clientfd = add_new_client(serverfd);
    int index = find_free(connections, max_clients);
    if (index == -1) {
        // disconnect the client
        fprintf(stderr, "[Server] Connection has been refused: too many clients.\n");
        char *buff = "Error: too many clients";
        if (bulk_write(clientfd, buff, strlen(buff)) < 0) {
            ERR("bulk_write");
        }

        if (TEMP_FAILURE_RETRY(close(clientfd) < 0)) {
            ERR("close");
        }

        return;
    }

    // prepare data for the new client
    connections[index].free = 0;
    connections[index].clientfd = clientfd;
    connections[index].offset = 0;
    connections[index].question_id = rand() % quiz_data->size;
    connections[index].msg_sent = 0;

    // send hello
    char *hello = "Hello!\n";
    if (bulk_write(clientfd, hello, strlen(hello)) < 0) {
        ERR("bulk_write");
    }
}

int create_rfds(fd_set *rfds, int serverfd, struct connection *connections, int max_clients) {
    FD_ZERO(rfds);
    FD_SET(serverfd, rfds);
    int maxfd = serverfd;
    for (int i = 0; i < max_clients; ++i) {
        if (connections[i].free) continue;

        FD_SET(connections[i].clientfd, rfds);

        if (maxfd < connections[i].clientfd) {
            maxfd = connections[i].clientfd;
        }
    }

    return maxfd;
}

void read_from_clients(struct connection *connections, int max_clients, fd_set *rfds, struct quiz *quiz_data)
{
    for (int i = 0; i < max_clients; ++i) {
        if (connections[i].free) continue;
        if (!connections[i].msg_sent) continue;
        if (!FD_ISSET(connections[i].clientfd, rfds)) continue;

        char buff;
        ssize_t size;
        if ((size = bulk_read(connections[i].clientfd, &buff, 1)) < 0) {
            if (size == 0) {
                // disconnect the client
                connections[i].free = 1;
                connections[i].offset = 0;
                connections[i].msg_sent = 0;
            } else {
                ERR("bulk_read");
            }
        }
        fprintf(stderr, "[Server] Client is ready for the answer!\n");
        if (bulk_write(connections[i].clientfd, quiz_data->answers[connections[i].question_id],
                       strlen(quiz_data->answers[connections[i].question_id])) < 0) {
            if (EPIPE == errno) {
                // disconnect the client
                connections[i].free = 1;
                connections[i].offset = 0;
                connections[i].msg_sent = 0;
                continue;
            } else {
                ERR("bulk_write");
            }
        }


        if (TEMP_FAILURE_RETRY(close(connections[i].clientfd)) < 0) {
            ERR("close");
        }

        fprintf(stderr, "[Server] Client has been disconnected.\n");

        connections[i].free = 1;
        connections[i].offset = 0;
        connections[i].msg_sent = 0;
    }
}

void write_to_clients(struct connection *connections, int max_clients, struct quiz *quiz_data)
{
    for (int i = 0; i < max_clients; ++i) {
        if (connections[i].free) continue;
        if (connections[i].msg_sent) continue;

        if (connections[i].offset >= strlen(quiz_data->questions[connections[i].question_id])) {
            fprintf(stderr, "[Server] Sending completed\n");
            connections[i].offset = 0;
            connections[i].msg_sent = 1;
            continue;
        }

        char byte = quiz_data->questions[connections[i].question_id][connections[i].offset];
        connections[i].offset++;
        if (bulk_write(connections[i].clientfd, &byte, 1) < 0) {
            if (EPIPE == errno) {
                // disconnect the client
                connections[i].free = 1;
                connections[i].offset = 0;
            } else {
                ERR("bulk_write");
            }
        }

    }
}

void do_server(int serverfd, int max_clients, struct quiz *quiz_data)
{
    // create table for clients
    struct connection *connections = (struct connection *)malloc(max_clients * sizeof(struct connection));
    if (connections == NULL) {
        ERR("malloc");
    }

    // initialize conection holders
    for (int i = 0; i < max_clients; ++i) {
        connections[i].free = 1;
        connections[i].offset = 0;
        connections[i].msg_sent = 0;
    }

    // allow clients flag
    int allow_clients = 1;

    // ignore SIGINT if pselect is not running
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGUSR1);
	sigprocmask(SIG_BLOCK, &mask, &oldmask);

    fd_set rfds, wfds;
    int fdmax;
    struct timespec timeout;

    // main loop
    while (do_work) {
        fdmax = create_rfds(&rfds, serverfd, connections, max_clients);

        // Upon successful completion, the select()  function  may  modify  the  object
        // pointed to by the timeout argument.
        timeout.tv_sec = 0;
        timeout.tv_nsec = 330000000;

        if (pselect(fdmax + 1, &rfds, NULL, NULL, &timeout, &oldmask) >= 0) {
            // new connection is pending
            if (FD_ISSET(serverfd, &rfds)) {
                new_client_event(serverfd, max_clients, quiz_data, connections);
            }

            read_from_clients(connections, max_clients, &rfds, quiz_data);
            write_to_clients(connections, max_clients, quiz_data);

        } else {
            fprintf(stderr, "%d\n",errno);
            if (EINTR == errno) {
                if (SIGUSR1 == last_signal)
                    allow_clients = 1 - allow_clients;
            } else {
                ERR("pselect");
            }
        }
    }

    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}