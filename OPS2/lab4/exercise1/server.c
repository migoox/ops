#define _GNU_SOURCE
#include "../../mysocklib/mysocklib.h"
#include <netinet/in.h>
#include <semaphore.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_CLIENTS 10

volatile sig_atomic_t do_work = 1;

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s  port\n", name);
}

void sigint_handler(int sigNo);
void do_server(int server_fd);
void *thread_work(void *arg);

struct thread_args {
    int server_fd;
    int16_t time;
    struct sockaddr_in client_addr;
    sem_t *semaphore;
};

int main(int argc, char** argv)
{
    int server_fd;
    if (argc != 2) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (sethandler(SIG_IGN, SIGPIPE))
        ERR("Setting PIPE failed");

    if (sethandler(sigint_handler, SIGINT))
        ERR("Setting SIGINT failed");

    // blocking mode
    server_fd = UDP_IPv4_bind_socket(atoi(argv[1]));

    do_server(server_fd);

    if (TEMP_FAILURE_RETRY(close(server_fd)) < 0)
        ERR("Cannot close server_fd");

    fprintf(stderr, "[Server] Terminated\n");

    return EXIT_SUCCESS;
}

void sigint_handler(int sigNo)
{
    do_work = 0;
}

void do_server(int server_fd)
{
    int16_t time_ordered;
    struct sockaddr_in client_addr;

    // this will be sent in the case there are too many clients
    int16_t deny = -1;
    socklen_t addr_len = sizeof(struct sockaddr_in);

    // create unnamed semaphore
    sem_t semaphore;
    if (sem_init(&semaphore, 0, MAX_CLIENTS))
        ERR("sem_init() failed");

    fprintf(stderr, "[Server] Started\n");

    while (do_work) {
        while (recvfrom(server_fd, (char *) (&time_ordered),
                                        sizeof (int16_t), 0, &client_addr, &addr_len) < 0)
        {
            if (errno == EINTR) {
                if (!do_work) return;
            } else
            {
                ERR("recvfrom() failed");
            }
        }

        fprintf(stderr, "[Server] Received time order\n");

        if (TEMP_FAILURE_RETRY(sem_trywait(&semaphore))) {
            if (errno == EAGAIN) {
                // semaphore is locked, meaning there is no place for another client
                // send message to the client
                fprintf(stderr, "[Server] Busy\n");

                if (TEMP_FAILURE_RETRY(sendto(server_fd, (char *) (&deny),
                                              sizeof(int16_t), 0, &client_addr,
                                              sizeof(client_addr))) < 0) {
                    // if it's not the case when the client is offline throw error
                    if (errno != EPIPE)
                        ERR("sendto() failed");
                }
                continue;
            }
            ERR("sem_trywait() failed");
        }

        // create detached pthread
        pthread_t tid;
        struct thread_args *args;
        // this should be deallocated in thread_work func
        if ((args = (struct thread_args*)malloc(sizeof(struct thread_args))) == NULL)
            ERR("malloc() failed");

        args->server_fd = server_fd;
        args->semaphore = &semaphore;
        args->client_addr = client_addr;
        args->time = ntohs(time_ordered);

        if (pthread_create(&tid, NULL, thread_work, (void*) args))
            ERR("pthread_create() failed");

        if (pthread_detach(tid))
            ERR("pthread_detach() failed");
    }
}

void *thread_work(void *arg)
{
    struct thread_args *args = (struct thread_args *) arg;
    int is_client_offline = 0;

    fprintf(stderr, "[Server] Thread will sleep for %d\n", args->time);
    bulk_nanosleep(args->time, 0);

    if (TEMP_FAILURE_RETRY(sendto(args->server_fd, (char *) (&args->time),
                                  sizeof(int16_t), 0, &args->client_addr,
                                  sizeof(args->client_addr))) < 0) {

        // if it's not the case when the client is offline throw error
        if (errno != EPIPE)
            ERR("sendto() failed");

        is_client_offline = 1;
    }

    if (is_client_offline)
        fprintf(stderr, "[Server] Client is offline\n");
    else
        fprintf(stderr, "[Server] Notification sent\n");

    if (TEMP_FAILURE_RETRY(sem_post(args->semaphore)))
        ERR("sem_post() failed");

    free(args);

    return NULL;
}