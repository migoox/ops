#define _GNU_SOURCE
#include <aio.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>

#ifndef _POSIX_ASYNCHRONOUS_IO
// error throws compilation error with the given message
#error System does not support asynchronous I/O
#endif

#define MAX_DIR_LENGTH 101
#define BUFFERS_COUNT 3

void usage(char *progname)
{
    fprintf(stderr, "USAGE: %s directory, block size\n", progname);
    exit(EXIT_FAILURE);
}

void error(char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void suspend(struct aiocb *aiocbs)
{
    struct aiocb *aiolist[1];
    aiolist[0] = aiocbs;

    while (aio_suspend((const struct aiocb *const *)aiolist, 1, NULL) == -1)
    {
        if (errno == EINTR)
            continue;

        error("Suspend error");
    }
    if (aio_error(aiocbs) != 0)
        error("Suspend error");
    if (aio_return(aiocbs) == -1)
        error("Return error");
}

void getfds(const char *path, int block_size,
            int **file_sizes, int **file_descriptors, int *files_count);
void init_buffers(char *buffers[BUFFERS_COUNT], int block_size);
void init_aiocbs(struct aiocb aiocbs[BUFFERS_COUNT], char *buffers[BUFFERS_COUNT],
                 int block_size, int *file_sizes, int *file_descriptors, int files_count);

void cleanupfds(int **file_sizes, int **file_descriptors, int files_count);
void cleanup_buffers(char *buffers[BUFFERS_COUNT]);

void readfiles(struct aiocb aiocbs[BUFFERS_COUNT]);
void writetofiles(struct aiocb aiocbs[BUFFERS_COUNT]);

void *worker(void *rawPtr);
void mainthread_wait(sigset_t *waitMask);

typedef struct worker_args
{
    struct aiocb *aiocbs;
    char **buffers;
    int block_size;
} worker_args_t;

int main(int argc, char **argv)
{
    if (argc != 3)
        usage(argv[0]);

    int block_size = atoi(argv[2]);
    if (block_size < 1)
        usage(argv[0]);
    const char *path = argv[1];

    // block all signals used in this program by deafult
    sigset_t newMask;
    sigemptyset(&newMask);
    sigaddset(&newMask, SIGUSR2);
    sigaddset(&newMask, SIGUSR1);
    sigaddset(&newMask, SIGCONT);
    if (pthread_sigmask(SIG_BLOCK, &newMask, NULL))
        error("SIG_BLOCK error");

    // mainthread waits only for SIGUSR1 signal
    sigset_t waitMask;
    sigemptyset(&waitMask);
    sigaddset(&waitMask, SIGUSR1);

    // prepare containers
    int *file_descriptors;
    int *file_sizes;
    int files_count;
    struct aiocb aiocbs[BUFFERS_COUNT];
    char *buffers[BUFFERS_COUNT];

    getfds(path, block_size, &file_sizes, &file_descriptors, &files_count);
    init_buffers(buffers, block_size);
    init_aiocbs(aiocbs, buffers, block_size, file_sizes, file_descriptors, files_count);

    pthread_t worker_tid;
    // init worker arguments
    worker_args_t w_args;
    w_args.aiocbs = aiocbs;
    w_args.buffers = buffers;
    w_args.block_size = block_size;

    // read files and suspend
    readfiles(aiocbs);

    // create worker thread
    if (pthread_create(&worker_tid, NULL, worker, &w_args))
        error("pthread_create() failed");

    // wait for the worker to finish
    mainthread_wait(&waitMask);

    // worker is waiting for further instructions

    for (int i = BUFFERS_COUNT; i < files_count; i += BUFFERS_COUNT)
    {
        // update aiobs
        for (int j = 0; j < BUFFERS_COUNT; j++)
        {
            if (i + j < files_count)
            {
                aiocbs[j].aio_fildes = file_descriptors[i + j];
                aiocbs[j].aio_nbytes = file_sizes[i + j];
            }
            else
            {
                aiocbs[j].aio_fildes = -1;
            }
        }

        // read files and suspend
        readfiles(aiocbs);

        // make the worker work
        printf("SIGCONT is sent from the mainthread.\n");
        kill(getpid(), SIGCONT);

        // wait for the worker to finish
        mainthread_wait(&waitMask);

        // worker is waiting for further instructions
    }

    // fire the worker
    printf("SIGUSR2 is sent from the mainthread.\n");
    kill(getpid(), SIGUSR2);

    // wait for the thread to join
    if (pthread_join(worker_tid, NULL))
        error("pthread_join() failed");

    // free resources
    cleanupfds(&file_sizes, &file_descriptors, files_count);
    cleanup_buffers(buffers);
    printf("Cleaned\n");

    return EXIT_SUCCESS;
}

void getfds(const char *path, int block_size,
            int **file_sizes, int **file_descriptors, int *files_count)
{
    printf("Directory scan result:\n");
    DIR *dir;
    struct dirent *entry;
    struct stat sb;

    *files_count = 0;

    // save current working directory
    char cwd[MAX_DIR_LENGTH];
    getcwd(cwd, MAX_DIR_LENGTH);

    // change directory to given path
    if (chdir(path) == -1)
        error("Unable to change directory");

    // open directory
    dir = opendir(".");
    if (dir == NULL)
        error("Unable to open the directory");

    // count files
    while ((entry = readdir(dir)) != NULL)
    {
        if (lstat(entry->d_name, &sb) == -1)
            error("lstat failed");
        if (sb.st_size > block_size)
            continue;
        *files_count += 1;
    }

    // allocate memory
    *file_descriptors = (int *)malloc(sizeof(int) * (*files_count));
    *file_sizes = (int *)malloc(sizeof(int) * (*files_count));

    // reopen direcotry
    closedir(dir);
    dir = opendir(".");
    if (dir == NULL)
        error("Unable to open the directory");

    // open files and save their files descriptors
    int i = 0;
    while ((entry = readdir(dir)) != NULL)
    {
        if (lstat(entry->d_name, &sb) == -1)
            error("lstat failed");
        if (sb.st_size > block_size)
            continue;

        int fd;
        if ((fd = open(entry->d_name, O_RDWR)) == -1)
            error("open failed");

        printf("name: %s, size: %ld, fd: %d\n", entry->d_name, sb.st_size, fd);
        (*file_descriptors)[i] = fd;
        (*file_sizes)[i++] = sb.st_size;
    }

    // close directory
    closedir(dir);

    // return to the current working directory
    if (chdir(cwd) == -1)
        error("Unable to change directory");
}

void cleanupfds(int **file_sizes, int **file_descriptors, int files_count)
{
    for (int i = 0; i < files_count; i++)
    {
        if (close((*file_descriptors)[i]))
            error("close failed");
    }

    free(*file_descriptors);
    free(*file_sizes);
}

void init_aiocbs(struct aiocb aiocbs[BUFFERS_COUNT], char *buffers[BUFFERS_COUNT],
                 int block_size, int *file_sizes, int *file_descriptors, int files_count)
{
    for (int i = 0; i < BUFFERS_COUNT; i++)
    {
        memset(&aiocbs[i], 0, sizeof(struct aiocb));

        if (i < files_count)
        {
            aiocbs[i].aio_fildes = file_descriptors[i];
            aiocbs[i].aio_nbytes = file_sizes[i];
        }
        else
        {
            aiocbs[i].aio_fildes = -1;
        }
        aiocbs[i].aio_offset = 0;
        aiocbs[i].aio_buf = buffers[i];
        aiocbs[i].aio_sigevent.sigev_notify = SIGEV_NONE;
    }
}

void init_buffers(char *buffers[BUFFERS_COUNT], int block_size)
{
    for (int i = 0; i < BUFFERS_COUNT; i++)
    {
        buffers[i] = calloc(block_size, sizeof(char));
        if (buffers[i] == NULL)
            error("calloc failed");
    }
}

void cleanup_buffers(char *buffers[BUFFERS_COUNT])
{
    for (int i = 0; i < BUFFERS_COUNT; i++)
        free(buffers[i]);
}

void readfiles(struct aiocb aiocbs[BUFFERS_COUNT])
{
    for (int i = 0; i < BUFFERS_COUNT; i++)
    {
        if (aiocbs[i].aio_fildes == -1)
            continue;

        if (aio_read(&aiocbs[i]) == -1)
            error("aio_read failed");
    }

    for (int i = 0; i < BUFFERS_COUNT; i++)
    {
        if (aiocbs[i].aio_fildes == -1)
            continue;

        suspend(&aiocbs[i]);
    }
}

void writetofiles(struct aiocb aiocbs[BUFFERS_COUNT])
{
    for (int i = 0; i < BUFFERS_COUNT; i++)
    {
        if (aiocbs[i].aio_fildes == -1)
            continue;

        // clear content of the file
        if (ftruncate(aiocbs[i].aio_fildes, 0) == -1)
            error("fruncate failed");

        if (aio_write(&aiocbs[i]) == -1)
            error("aio_read failed");
    }

    for (int i = 0; i < BUFFERS_COUNT; i++)
    {
        if (aiocbs[i].aio_fildes == -1)
            continue;

        suspend(&aiocbs[i]);
        // sync
        if (aio_fsync(O_SYNC, &aiocbs[i]) == -1)
            error("Cannot sync\n");
    }
}

void *worker(void *rawPtr)
{
    worker_args_t *args = (worker_args_t *)rawPtr;

    // block all signals used in this program by deafult
    sigset_t oldMask, newMask;
    sigemptyset(&newMask);
    sigaddset(&newMask, SIGUSR1);
    sigaddset(&newMask, SIGUSR2);
    sigaddset(&newMask, SIGCONT);
    if (pthread_sigmask(SIG_BLOCK, &newMask, &oldMask))
        error("SIG_BLOCK error");

    // worker waits only for SIGUSR1 signal
    sigset_t waitMask;
    sigemptyset(&waitMask);
    sigaddset(&waitMask, SIGCONT);
    sigaddset(&waitMask, SIGUSR2);

    // 1 is added to get additional cell for '\0'
    char *temp_buf = malloc((args->block_size + 1) * sizeof(char));
    if (temp_buf == NULL)
        error("calloc failed");

    const char *consonants = "bcdfgjklmnpqstvxzhrwBCDFGJKLMNPQSTVXZHRW";
    int signo = -1;
    while (1)
    {
        printf("\n");
        for (int i = 0; i < BUFFERS_COUNT; i++)
        {
            if (args->aiocbs[i].aio_fildes == -1)
                continue;

            // delete vovels
            memcpy(temp_buf, args->buffers[i], args->aiocbs[i].aio_nbytes);
            temp_buf[args->aiocbs[i].aio_nbytes] = '\0';

            printf("[fd=%d] before: %s\n", args->aiocbs[i].aio_fildes, temp_buf);

            char *ptr = temp_buf;
            int new_size = 0;
            while ((ptr = strpbrk(ptr, consonants)) != NULL)
            {
                temp_buf[new_size++] = *ptr;
                ptr++;
            }
            temp_buf[new_size] = '\0';

            printf("[fd=%d] after:  %s\n", args->aiocbs[i].aio_fildes, temp_buf);

            memcpy(args->buffers[i], temp_buf, new_size);
            args->aiocbs[i].aio_nbytes = new_size;
        }

        // aio_write
        writetofiles(args->aiocbs);

        // inform mainthread that work is finished
        printf("SIGUSR1 is sent to the main thread.\n");
        kill(getpid(), SIGUSR1);

        // wait for the mainthread's response
        if (sigwait(&waitMask, &signo))
            error("sigwait failed");

        if (signo == SIGUSR2)
        {
            // finish the work
            printf("SIGUSR2 is received.\n");
            break;
        }
        else if (signo == SIGCONT)
        {
            // do another task
            printf("SIGCONT is received.\n");
        }
        else
        {
            printf("UKNOWN is received.\n");
            break;
        }

    }

    free(temp_buf);
    return NULL;
}

void mainthread_wait(sigset_t *waitMask)
{
    int signo;
    if (sigwait(waitMask, &signo))
        error("sigwait failed");

    if (signo == SIGUSR1)
        printf("SIGUSR1 is received.\n");
    else
        printf("UKNOWN is received.\n");
}