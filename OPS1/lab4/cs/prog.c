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

volatile sig_atomic_t work;

void error(char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void sethandler(void (*f)(int), int sig)
{
	struct sigaction sa;
	memset(&sa, 0x00, sizeof(struct sigaction));
	sa.sa_handler = f;
	if (sigaction(sig, &sa, NULL) == -1)
		error("Error setting signal handler");
}

void ALRM_handler(int sig)
{
    work = 0;
}

void usage(char *progname)
{
    fprintf(stderr, "USAGE: %s filename, block_size\n", progname);
    exit(EXIT_FAILURE);
}

int suspend(struct aiocb *aiocbs)
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

    int ret = aio_return(aiocbs);
    if (ret == -1)
        error("Return error");

    return ret;
}

off_t getfilelength(int filedes)
{
    struct stat buff;

    // The  fstat()  function  shall  
    // obtain information about an open file associated 
    // with the file descriptor fildes.
    if (fstat(filedes, &buff) == -1)
        error("Cannot fstat the file");
            
    // stat structure contains information about size
    // of the file
    return buff.st_size;
}

typedef struct worker_args
{
    char* buffer;
    struct aiocb* console;
    int size;
} worker_args_t;

void *worker(sigval_t sigval)
{
    printf("WORKER: \n");
    worker_args_t* args = (worker_args_t*) sigval.sival_ptr;
    for(int i = 0; i < args->size; i++)
    {
        if (!((args->buffer[i] >= 'a' && args->buffer[i] <= 'z') || 
              (args->buffer[i] >= 'A' && args->buffer[i] <= 'Z') ))
              args->buffer[i] = ' ';
    }

    // if (aio_write(args->console) < 0)
    //     error("aio_write failed");

    // suspend(args->console);

    printf("\n");
    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 3)
        usage(argv[0]);
    
    work = 1;
    sethandler(ALRM_handler, SIGALRM);

    // read input
    int block_size = atoi(argv[2]);
    if (block_size < 1)
        usage(argv[0]);
    const char* filename = argv[1];

    int fd;
    if ((fd = open(filename, O_RDWR)) < 0)
        error("open failed");

    char* buffer;
    if ((buffer = (char*)malloc(sizeof(char) * block_size)) == NULL)
        error("malloc failed");

    int file_size = getfilelength(fd);
    int iterations = file_size / block_size;
    int remainder = file_size - iterations * block_size;

    struct aiocb console;
    memset(&console, 0, sizeof(struct sigevent));
    console.aio_fildes = 1;
    console.aio_offset = 0;
    console.aio_buf = buffer;
    console.aio_nbytes = block_size;
    console.aio_sigevent.sigev_notify = SIGEV_NONE;

    struct aiocb r;
    memset(&r, 0, sizeof(struct sigevent));
    r.aio_fildes = fd;
    r.aio_offset = 0;
    r.aio_buf = buffer;
    r.aio_nbytes = block_size;
    r.aio_sigevent.sigev_notify = SIGEV_THREAD;
    r.aio_sigevent.sigev_notify_function = worker;
    worker_args_t w_args;
    w_args.buffer = buffer;
    w_args.console = &console;
    w_args.size = block_size;
    r.aio_sigevent.sigev_value.sival_ptr = &w_args;

    alarm(3);
    for (int i = 0; i < iterations; i++)
    {
        if (!work)
            break;
        if (aio_read(&r) < 0)
            error("aio_read failed");
        suspend(&r);
        r.aio_offset += block_size;
    }

    // remainder
    // if (remainder > 0 && work)
    // {
    //     r.aio_nbytes = remainder;
    //     w_args.size = remainder;
        
    //     if (aio_read(&r) < 0)
    //         error("aio_read failed");

    //     suspend(&r);
    // }

    free(buffer);
    close(fd);

    return EXIT_SUCCESS;
}