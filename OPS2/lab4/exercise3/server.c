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
#define MAX_CLIENTS 3

volatile sig_atomic_t do_work = 1;

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s port\n", name);
}

typedef struct thread_args{
    int id;
    pthread_t *tids;
    int client_fd;
    pthread_barrier_t *barrier;
    pthread_cond_t *cond_var;
    pthread_mutex_t *cond_mtx;
    int *last_id;
    char *last_letter;
    sem_t *sem;
    int *thread_count;
    pthread_mutex_t *thread_count_mtx;
} thread_args_t;

void sigint_handler(int sigNo);

void do_server(int server_fd);

// thread
void *handle_client(void* arg);
void first_stage(thread_args_t* args);
void second_stage(thread_args_t* args);

// cleanups
void main_cleanup(void* arg);
void mtx_cleanup(void* arg);
void barrier_cleanup(void* arg);

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

    // non blocking mode tcp ipv4
    server_fd = TCP_IPv4_bind_socket(atoi(argv[1]), BACKLOG);
    int new_flags = fcntl(server_fd, F_GETFL) | O_NONBLOCK;
    if (fcntl(server_fd, F_SETFL, new_flags) == -1)
        ERR("fcntl");

    do_server(server_fd);

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

void do_server(int server_fd)
{
    pthread_t tids[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; ++i)
        tids[i] = -1;

    pthread_mutex_t  thread_count_mtx = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t cond_mtx = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond_var = PTHREAD_COND_INITIALIZER;

    pthread_barrier_t barrier;
    if (pthread_barrier_init(&barrier, NULL, MAX_CLIENTS))
        ERR("pthread_barrier_init()");

    int thread_count = 0;
    int last_id = 0;
    char last_letter = 'A';

    // initialize unnamed semaphroe
    sem_t thrds_sem;
    if (sem_init(&thrds_sem,0,MAX_CLIENTS))
        ERR("sem_init()");

    // descriptors for pselect (pselect blocks in order to wait for incoming clients connections)
    fd_set base_rfds, rfds;
    FD_ZERO(&base_rfds);
    FD_SET(server_fd, &base_rfds);

    // SIGINT and SIGUSR1 will be ignoered if pselect is not running
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    fprintf(stderr, "[Server] Started\n");

    while (do_work) {
        rfds = base_rfds;
        if (pselect(server_fd + 1, &rfds, NULL, NULL, NULL, &oldmask) != -1) {

            // new client connection is pending
            int client_fd;
            if ((client_fd = add_new_client(server_fd)) == -1)
                continue;

            // order one thread for the client if it is possible
            if (sem_trywait(&thrds_sem) == -1) {
                if (errno == EAGAIN) {
                    // the semaphore was already locked
                    // send an information about full server to the client
                    char buff = '0';
                    if (TEMP_FAILURE_RETRY(send(client_fd, &buff, sizeof(char), 0))) {
                        // ignore error caused by situation when the client is disconnected
                        if (errno != EPIPE)
                            ERR("send()");
                    }
                    continue;
                } else {
                    ERR("sem_trywait()");
                }
            }

            // initialize arguments for the new thread handling a client
            thread_args_t *args;
            if ((args = (thread_args_t*)malloc(sizeof(thread_args_t))) == NULL)
                ERR("malloc()");
            args->client_fd = client_fd;
            args->sem = &thrds_sem;
            args->thread_count = &thread_count;
            args->thread_count_mtx = &thread_count_mtx;
            args->cond_mtx = &cond_mtx;
            args->cond_var = &cond_var;
            args->last_id = &last_id;
            args->last_letter = &last_letter;
            args->barrier = &barrier;
            args->tids = tids;

            // increment the thread count
            if (pthread_mutex_lock(args->thread_count_mtx))
                ERR("pthread_mutex_lock()");
            args->id = ++thread_count;

            if (pthread_mutex_unlock(args->thread_count_mtx))
                ERR("pthread_mutex_unlock()");

            // create a new thread for the client
            pthread_t tid = -1;
            if (pthread_create(&tid, NULL, handle_client, (void *)args))
                ERR("pthread_create");
            tids[args->id - 1] = tid;

            // detach the thread
            if (pthread_detach(tid))
                ERR("pthread_detach");


        } else if (errno != EINTR) {
                ERR("pselect()");
        }
    }

    if (sem_destroy(&thrds_sem))
        ERR("sem_destroy()");

    if (pthread_mutex_destroy(&thread_count_mtx))
        ERR("pthread_mutex_destroy()");

    if (pthread_mutex_destroy(&cond_mtx))
        ERR("pthread_mutex_destroy()");

    if (pthread_cond_destroy(&cond_var))
        ERR("pthread_cond_destroy()");
}

void *handle_client(void* arg)
{
    thread_args_t *args = (thread_args_t*) arg;

    // push the main cleanup
    pthread_cleanup_push(main_cleanup, (void *) args);

    /* FIRST STATE - WAITING FOR ALL THE CLIENTS */
    first_stage(args);

    pthread_cleanup_push(barrier_cleanup, (void *) args);

    pthread_cleanup_push(mtx_cleanup, (void *) args);

    /* SECOND STATE - ,,LITEROWANIE'' */
    second_stage(args);

    if (args->id == MAX_CLIENTS) {
        // it is the last thread - reset values
        // all the other threads are blocked at this point, so this access is valid
        *args->last_id = 0;
        *args->last_letter = 'A';
    } else {
        // wakeup the next thread
        pthread_cond_broadcast(args->cond_var);
    }

    // unlock the mutex by poping mtx_cleanup
    pthread_cleanup_pop(1);

    // call the barrier
    pthread_cleanup_pop(1);

    // to main cleanup
    pthread_cleanup_pop(1);

    return NULL;
}

void first_stage(thread_args_t* args)
{
    // control message
    char buff = '1';

    while (1) {
        // break the loop if there are MAX_CLIENTS clients
        if (pthread_mutex_lock(args->thread_count_mtx))
            ERR("pthread_mutex_lock()");
        if (*args->thread_count >= MAX_CLIENTS) {
            if (pthread_mutex_unlock(args->thread_count_mtx))
                ERR("pthread_mutex_unlock()");
            break;
        }
        if (pthread_mutex_unlock(args->thread_count_mtx))
            ERR("pthread_mutex_unlock()");

        // send control message to the client
        if (TEMP_FAILURE_RETRY(send(args->client_fd, &buff, sizeof(char), 0)) == -1) {
            // ignore error caused by situation when the client is disconnected
            if (errno == EPIPE) {
                // call the cleanup func and terminate the thread
                pthread_exit(NULL);
            } else {
                ERR("send()");
            }
        }

        // sleep 1 sec before the next notification
        bulk_nanosleep(1, 0);
    }
}

void second_stage(thread_args_t* args)
{
    if (pthread_mutex_lock(args->cond_mtx))
        ERR("pthread_mutex_lock()");

    while (*args->last_id + 1 != args->id) {
        // The application shall ensure that pthread_cond_wait function is called
        // with the mutex locked by the calling thread (manual)
        pthread_cond_wait(args->cond_var, args->cond_mtx);
    }

    *args->last_id = args->id;

    char letter = '0';
    char waiting_for = (char)((*args->last_letter + 1) % 'A' + 'A');

    fprintf(stderr, "[Server] Waiting for %c\n", waiting_for);

    if (TEMP_FAILURE_RETRY(send(args->client_fd, args->last_letter, sizeof(char), 0)) == -1) {
        // ignore error caused by situation when the client is disconnected (it will be checked in recv)
        if (errno != EPIPE)
            ERR("send()");
    }

    while (letter != waiting_for) {
        ssize_t size;
        if (TEMP_FAILURE_RETRY(size = (recv(args->client_fd, &letter, sizeof(char), 0))) == -1)
            ERR("recv()");

        if (size == 0) {
            // eof detected => cancel threads
            for (int i = 1; i <= MAX_CLIENTS; ++i) {
                if (i == args->id) continue;
                if (pthread_cancel(args->tids[i - 1]))
                    ERR("pthread_cancel()");
            }

            pthread_exit(NULL);
        }
    }

    *args->last_letter = letter;
}

void main_cleanup(void* arg)
{
    thread_args_t *args = (thread_args_t*) arg;

    fprintf(stderr, "[Server] Cleanup %d\n", args->id);

    // decrement the thread count
    if (pthread_mutex_lock(args->thread_count_mtx))
        ERR("pthread_mutex_lock()");
    (*args->thread_count)--;
    if (pthread_mutex_unlock(args->thread_count_mtx))
        ERR("pthread_mutex_unlock()");

    // resource (thread) should be available from now
    sem_post(args->sem);

    if (TEMP_FAILURE_RETRY(close(args->client_fd)))
        ERR("close()");

    free(args);

    printf("[Server] Thread finished succesfuly");
}

void mtx_cleanup(void* arg)
{
    thread_args_t *args = (thread_args_t*) arg;
    if (pthread_mutex_unlock(args->cond_mtx))
        ERR("pthread_mutex_unlock()");
}

void barrier_cleanup(void* arg)
{
    thread_args_t *args = (thread_args_t*) arg;
    fprintf(stderr, "[Server] On barrier - %d\n", args->id);
    // wait for all threads to reach that point
    int result;
    if ((result = pthread_barrier_wait(args->barrier))) {
        // the constant PTHREAD_BARRIER_SERIAL_THREAD shall be returned to
        // one  unspecified  thread  and  zero  shall  be returned to each of the remaining
        // threads (manual)
        if (result != PTHREAD_BARRIER_SERIAL_THREAD)
            ERR("pthread_barrier_wait()");
    }

    // NIE WIEM JAKIM CUDEM, ALE JAK NIE MA TEGO PRINTA TO NIE DZIALA
    fprintf(stderr, "[Server] Barrier end - %d\n", args->id);
}