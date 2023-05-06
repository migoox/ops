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

#define BLOCKS 3
#define EIGHT 8
#define BUFFERS 6
#define SHIFT(counter, x) ((counter + x) % BLOCKS)
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void suspend(struct aiocb *aiocbs)
{
    struct aiocb *aiolist[1];
    aiolist[0] = aiocbs;

    while (aio_suspend((const struct aiocb *const *)aiolist, 1, NULL) == -1)
    {
        if (errno == EINTR)
            continue;

        ERR("Suspend error");
    }
    if (aio_error(aiocbs) != 0)
        ERR("Suspend error");
    if (aio_return(aiocbs) == -1)
        ERR("Return error");
}

void error(char *);
void usage(char *);
void cipher(char *text, char *key, char *result, int size);

volatile sig_atomic_t work;

void cipher(char *text, char *key, char *result, int size) 
{
    for (int i = 0; i < size; i++) 
    {
        result[i] = ((text[i] - 'A') + (key[i] - 'A')) % ('Z' - 'A' + 1) + 'A';
    }
}
void error(char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

void usage(char *progname)
{
	fprintf(stderr, "text\n");
	fprintf(stderr, "key\n");
	fprintf(stderr, "result\n");
	exit(EXIT_FAILURE);
}

off_t getfilelength(int fd)
{
	struct stat buf;
	if (fstat(fd, &buf) == -1)
		error("Cannot fstat file");
	return buf.st_size;
}

int main(int argc, char *argv[])
{
	if(argc!=4)
        usage(argv[0]);
    
    int textFd, keyFd, resultFd;
    if ((textFd = open(argv[1], O_RDONLY)) < 0|| 
        (keyFd = open(argv[2], O_RDONLY)) < 0 ||
        (resultFd = open(argv[3], O_WRONLY)) < 0)
        ERR("OPEN");

    printf("%d\n", resultFd);
    printf("%d\n", textFd);
    printf("%d\n", keyFd);

    struct stat buf;
	if (fstat(textFd, &buf) == -1)
		error("Cannot fstat file");
    int text_size=buf.st_size;

    if (fstat(keyFd, &buf) == -1)
		error("Cannot fstat file");
    int key_size=buf.st_size;
    
    struct aiocb *textAiocb = malloc(sizeof(struct aiocb));
    if (textAiocb == NULL)
        ERR("MALLOC");
    char *buffer = calloc(text_size, sizeof(char));
    if (!buffer)
        ERR("MALLOC");
    memset(textAiocb, 0, sizeof(struct aiocb));
    textAiocb->aio_fildes = textFd;
    textAiocb->aio_buf = buffer;
    textAiocb->aio_nbytes = text_size;
    textAiocb->aio_offset = 0;

    struct aiocb *keyAiocb = malloc(sizeof(struct aiocb));
    if (keyAiocb == NULL)
        ERR("MALLOC");
    char *bufferKey = calloc(key_size, sizeof(char));
    if (!bufferKey)
        ERR("MALLOC");
    memset(keyAiocb, 0, sizeof(struct aiocb));
    keyAiocb->aio_fildes = keyFd;
    keyAiocb->aio_buf = bufferKey;
    keyAiocb->aio_nbytes = key_size;
    keyAiocb->aio_offset = 0;

    struct aiocb *resultAiocb = malloc(sizeof(struct aiocb));
    if (resultAiocb == NULL)
        ERR("MALLOC");
    char *bufferResult = calloc(text_size, sizeof(char));
    if (!bufferResult)
        ERR("MALLOC");

    memset(resultAiocb, 0, sizeof(struct aiocb));
    resultAiocb->aio_fildes = resultFd;
    resultAiocb->aio_buf = bufferResult;
    resultAiocb->aio_nbytes = text_size;
    resultAiocb->aio_offset = 0;

    if (aio_read(textAiocb) < 0 || aio_read(keyAiocb)<0) // read(fd, buffer, 100000);
        perror("aio_read()");

    suspend(textAiocb);
    suspend(keyAiocb);
    
    for(int i=0;i<key_size;i++)
    {
        fprintf(stderr, "%c",bufferKey[i]);
    }
        fprintf(stderr, "\n");

    for(int i=0;i<text_size;i++)
    {
        fprintf(stderr, "%c",buffer[i]);
    }
        fprintf(stderr, "\n");

    for(int i=0;i<text_size;i++)
    {
        bufferResult[i] = ((buffer[i] - 'A') + (bufferKey[i % key_size] - 'A')) % ('Z' - 'A' + 1) + 'A';
        fprintf(stderr, "%c",bufferResult[i]);
    }

    fprintf(stderr, "\nDUPA\n");

    if(aio_write(resultAiocb)<0) 
        ERR("aio_write");

    suspend(resultAiocb);

    if (aio_fsync(O_SYNC, resultAiocb) == -1)
        ERR("Cannot sync\n");

    fprintf(stderr, "\nDUPA\n");

    free(buffer);
    free(bufferKey);
    free(bufferResult);
    free(textAiocb);
    free(keyAiocb);
    free(resultAiocb);

    close(textFd);
    close(keyFd);
    close(resultFd);
}