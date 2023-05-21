#define _GNU_SOURCE
#include "../../mysocklib/mysocklib.h"
#include <netinet/in.h>
#include <semaphore.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

#define BACKLOG 3
#define CHUNK_SIZE 500
#define NMMAX 30
#define THREAD_COUNT 3
#define ERRSTRING "No such file or directory\n"

volatile sig_atomic_t do_work = 1;

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s port workdir\n", name);
}

typedef struct thread_args{
    int id;
    pthread_t tid;
    int *cfd;
    int *idlethreads;
    int *condition;
    pthread_mutex_t *mutex; // protects cfd and idlethreads values
    pthread_cond_t *cond;
} thread_args_t;

void sigint_handler(int sigNo);
void do_server(int server_fd, thread_args_t threads[THREAD_COUNT], int *idlethreads, int *cfd, int *condition, pthread_cond_t *cond, pthread_mutex_t *mutex);
void *thread_work(void *arg);
void cleanup(void *arg);
void communicate(int client_fd);

int main(int argc, char** argv)
{
    int server_fd;
    if (argc != 3) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    // change working directory to the given one
    if (chdir(argv[2]) == -1)
        ERR("chdir");

    if (sethandler(SIG_IGN, SIGPIPE))
        ERR("Setting PIPE failed");

    if (sethandler(sigint_handler, SIGINT))
        ERR("Setting SIGINT failed");

    // non blocking mode tcp ipv4
    server_fd = TCP_IPv4_bind_socket(atoi(argv[1]), BACKLOG);
    int new_flags = fcntl(server_fd, F_GETFL) | O_NONBLOCK;
    if (fcntl(server_fd, F_SETFL, new_flags) == -1)
        ERR("fcntl");

    // condition value and the mutex
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    int idlethreads = 0;
    int cfd = -1;
    int condition = 0;

    // init threads
    thread_args_t threads[THREAD_COUNT];

    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads[i].id = i + 1;
        threads[i].cfd = &cfd;
        threads[i].idlethreads = &idlethreads;
        threads[i].condition = &condition;
        threads[i].cond = &cond;
        threads[i].mutex = &mutex;

        if (pthread_create(&threads[i].tid, NULL, thread_work, (void*) &threads[i]))
            ERR("pthread_create() failed");
    }

    // start working
    do_server(server_fd, threads, &idlethreads, &cfd, &condition, &cond, &mutex);

    // first variant
//    condition = 1;
//    if (pthread_cond_broadcast(&cond))
//        ERR("pthread_cond_broadcast() failed");

    // second variant (cond_wait is cancelation point)
    for (int i = 0; i < THREAD_COUNT; ++i) {
        if (pthread_cancel(threads[i].tid))
            ERR("pthread_cancel() failed");
    }


    // join the working threads before leaving
    for (int i = 0; i < THREAD_COUNT; ++i) {
        if (pthread_join(threads[i].tid, NULL))
            ERR("pthread_join() failed");
    }

    // close the server
    if (TEMP_FAILURE_RETRY(close(server_fd)) < 0)
        ERR("Cannot close server_fd");

    fprintf(stderr, "[Server] Terminated\n");

    return EXIT_SUCCESS;
}

void sigint_handler(int sigNo)
{
    do_work = 0;
}

void do_server(int server_fd, thread_args_t threads[THREAD_COUNT], int *idlethreads, int *cfd, int *condition, pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    int client_fd;
    fd_set base_rfds, rfds;
    FD_ZERO(&base_rfds);
    FD_SET(server_fd, &base_rfds);

    // ignore SIGINT if pselect is not running
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    fprintf(stderr, "[Server] Started\n");

    while (do_work) {
        rfds = base_rfds;
        if (pselect(server_fd + 1, &rfds, NULL, NULL, NULL, &oldmask) != -1) {
            // new client connection is pending
            if ((client_fd = add_new_client(server_fd)) == -1)
                continue;

            if (pthread_mutex_lock(mutex))
                ERR("pthread_mutex_lock() failed");

            fprintf(stderr, "[Server] New client\n");

            // if there are no resources, disconnect the client and unlock the mutex
            if (*idlethreads == 0) {
                if (TEMP_FAILURE_RETRY(close(client_fd)))
                    ERR("close() failed");

                fprintf(stderr, "[Server] Client expelled, no resources\n");

                if (pthread_mutex_unlock(mutex) != 0)
                    ERR("pthread_mutex_unlock() failed");
            } else {
                *cfd = client_fd;
                if (pthread_mutex_unlock(mutex))
                    ERR("phtread_mutex_unlock() failed");

                *condition = 1;
                fprintf(stderr, "[Server] The client gets theirs thread \n");

                if (pthread_cond_signal(cond))
                    ERR("pthread_cond_signal() failed");
            }

        } else if (errno != EINTR) {
            ERR("pselect() failed");
        }
    }
}

void *thread_work(void *arg)
{
    int client_fd;
    thread_args_t args;
    memcpy(&args, arg, sizeof(thread_args_t));

    while (1) {
        pthread_cleanup_push(cleanup, (void *) args.mutex);
        if (pthread_mutex_lock(args.mutex))
            ERR("pthread_mutex_lock() failed");

        (*args.idlethreads)++;

        // block until the condition is not met
        while (!(*args.condition) && do_work) {
            // blocks and unlocks the mutex
            if (pthread_cond_wait(args.cond, args.mutex) != 0)
                ERR("pthread_cond_wait() failed");
        }

        if (!do_work) {
            pthread_exit(NULL); // it also pops the cancelation cleanup routine
        }

        // should be after the if statement
        *args.condition = 0;

        (*args.idlethreads)--;

        client_fd = *args.cfd;
        // pop and invoke the cleanup which simply unlocks the mutex
        pthread_cleanup_pop(1);
        communicate(client_fd);
    }

    return NULL;
}

void cleanup(void *arg)
{
    pthread_mutex_unlock((pthread_mutex_t *)arg);
}

void communicate(int client_fd)
{
    fprintf(stderr, "[Server] Starting communication with the client \n");

    ssize_t size;
    char filepath[NMMAX + 1];
    char buffer[CHUNK_SIZE];

    // On  SOCK_STREAM  sockets  MSG_WAITALL requests that the function
    // block until the full amount of data can be returned (man 3p recv).
    if ((size = TEMP_FAILURE_RETRY(recv(client_fd, filepath, NMMAX + 1, MSG_WAITALL))) == -1)
        ERR("recv() failed");

    // at this point size is 0 (eof) or NMMAX + 1
    if (size == NMMAX + 1) {
        int fd;
        // try to open the given file
        if ((fd = TEMP_FAILURE_RETRY(open(filepath, O_RDONLY))) == -1) {
            sprintf(buffer, ERRSTRING);
        } else  {
            memset(buffer, 0, CHUNK_SIZE);

            // eof can be detected
            if (bulk_read(fd, buffer, CHUNK_SIZE) == -1)
                ERR("read() failed");
        }

        // send read data
        if (TEMP_FAILURE_RETRY(send(client_fd, buffer, CHUNK_SIZE, 0)) == -1)
            ERR("write()");
    }

    if (TEMP_FAILURE_RETRY(close(client_fd)) < 0)
        ERR("close() failed");
}