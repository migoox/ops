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

#define FILES_COUNT 3
#define BLOCK_SIZE 8

void usage(char *progname)
{
    fprintf(stderr, "USAGE: %s in_file, key_file, out_file\n", progname);
    exit(EXIT_FAILURE);
}

void error(char *msg)
{
    perror(msg);
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

void init_aiocbs(struct aiocb aiocbs[FILES_COUNT], int fds[FILES_COUNT], char buffers[FILES_COUNT][BLOCK_SIZE])
{
    for (int i = 0; i < FILES_COUNT; i++)
    {
        memset(buffers[i], 0, BLOCK_SIZE);
        aiocbs[i].aio_fildes = fds[i];
        aiocbs[i].aio_buf = buffers[i];
        aiocbs[i].aio_nbytes = BLOCK_SIZE;
        aiocbs[i].aio_offset = 0;
        aiocbs[i].aio_sigevent.sigev_notify = SIGEV_NONE;
    }
}

void cipher(char *text_buffer, char *out_buffer, int size, char *key_buffer, int key_size)
{
    for (int i = 0; i < size; i++)
        out_buffer[i] = ((text_buffer[i] - 'A') + (key_buffer[i % key_size] - 'A')) % ('Z' - 'A' + 1) + 'A';
}

int main(int argc, char **argv)
{
    if (argc != 4)
        usage(argv[0]);

    // 0 - in1, 1 - key1, 2 - in2, 3 - key2, 4 - out
    int fds[FILES_COUNT];
    if ((fds[0] = open(argv[1], O_RDONLY)) < 0 || 
        (fds[1] = open(argv[2], O_RDONLY)) < 0 ||
        (fds[2] = open(argv[3], O_WRONLY || O_TRUNC)) < 0)
        error("open() failed");

    char buffers[FILES_COUNT][BLOCK_SIZE];
    char ciphered[BLOCK_SIZE];
    struct aiocb aiocbs[FILES_COUNT];
    init_aiocbs(aiocbs, fds, buffers);

    int text_size = getfilelength(fds[0]);
    int key_size = getfilelength(fds[1]);
    // assumption: key_size = text_size

    aio_read(&aiocbs[0]);
    aio_read(&aiocbs[1]);
    int text_bytes = suspend(&aiocbs[0]);
    int key_bytes = suspend(&aiocbs[1]);
    cipher(buffers[0], ciphered, text_bytes, buffers[1], key_bytes);
    int text_offset = text_bytes;
    int key_offset = key_bytes;

    aio_read(&aiocbs[0]);
    aio_read(&aiocbs[1]);
    text_bytes = suspend(&aiocbs[0]);
    key_bytes = suspend(&aiocbs[1]);
    text_offset += text_bytes;
    key_offset += key_bytes;

    for (int i = 0; key_bytes > 0; i++)
    {
        memcpy(buffers[4], ciphered, text_bytes);
        
        // write out
        aiocbs[4].aio_offset = text_offset;
        aio_write(&aiocbs[4]);

        // read in
        aiocbs[].aio_offset = text_offset;
        aio_read(&aiocbs[curr]);

        // read key
        aiocbs[curr + 1].aio_offset = text_offset;
        aio_read(&aiocbs[curr + 1]);

        // cipher
        cipher(buffers[next], ciphered, text_bytes, 
                buffers[next + 1], key_bytes);

        text_bytes = suspend(&aiocbs[curr]);
        key_bytes = suspend(&aiocbs[curr + 1]);
        text_offset += text_bytes;
        key_bytes += key_offset;

        suspend(&aiocbs[4]);
        fsync(&aiocbs[4]);
    }

    memcpy(buffers[4], ciphered, text_bytes);
        
    // write out
    aiocbs[4].aio_offset = text_offset;
    aio_write(&aiocbs[4]);

    return EXIT_SUCCESS;
}