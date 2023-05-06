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

// error throws compilation error with the given message
#ifndef _POSIX_ASYNCHRONOUS_IO
#error System does not support asynchronous I/O
#endif
// if we want to allow multiplatform ability we should
// provide using ifndef and endif directives different implementaions 
// based on the platform since AIO is not 100% portable between
// POSIX systems

#define BUFFERS_COUNT 3
#define SHIFT(counter, x) ((counter + x) % BUFFERS_COUNT)

volatile sig_atomic_t SIGINT_received;
volatile int aio_status;

void usage(char *progname)
{
	fprintf(stderr, "%s workfile blocksize\n", progname);
	fprintf(stderr, "workfile - path to the file to work on\n");
	fprintf(stderr, "n - number of blocks (>=2)\n");
	fprintf(stderr, "k - number of iterations (>=2)\n");
	exit(EXIT_FAILURE);
}

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

void suspend(struct aiocb *aiocbs)
{
	struct aiocb *aiolist[1];
	aiolist[0] = aiocbs;

	if (SIGINT_received)
		return;

	while (aio_suspend((const struct aiocb *const *)aiolist, 1, NULL) == -1) 
    {
		if (SIGINT_received)
			return;

		if (errno == EINTR)
			continue;

		error("Suspend error");
	}
	if (aio_error(aiocbs) != 0)
		error("Suspend error");
	if (aio_return(aiocbs) == -1)
		error("Return error");
}

void readdata(struct aiocb* aiocb, off_t offset)
{
    if (SIGINT_received)
        return;

    aiocb->aio_offset = offset;

    if (aio_read(aiocb))
        error("Couldn't read the file");
}

void writedata(struct aiocb* aiocb, off_t offset)
{
    if (SIGINT_received)
        return;

    aiocb->aio_offset = offset;

    if (aio_write(aiocb))
        error("Couldn't write to the file");
}

void syncdata(struct aiocb *aiocbs)
{
    if (SIGINT_received)
        return;

    // wait for end of writing
    suspend(aiocbs);
    // sync
    if (aio_fsync(O_SYNC, aiocbs) == -1)
		error("Cannot sync\n");
    // wait for end of syncing
    suspend(aiocbs);
}

void SIGINT_handler(int sig);
void choose_blocks(int *block1, int *block2, int max);
off_t getfilelength(int filedes);
void create_buffers_and_fillaiostructs(char* buffers[BUFFERS_COUNT], int block_size,
    struct aiocb* aiocbs, int filedes);
void cleanup(char *buffers[BUFFERS_COUNT], int filedes);
void reverse_buffer(char* buffer, int size);
void processblocks(struct aiocb *aiocbs, char **buffer, 
    int blocks_count, int block_size, int iterations_count);


int main(int argc, char **argv)
{
    // prepare input data 
    if (argc != 4) 
        usage(argv[0]);

    const char* workfile_path = argv[1];

    int blocks_count = atoi(argv[2]);
    int iteraions_count = atoi(argv[3]);
    if (blocks_count < 2 || iteraions_count < 2) 
        usage(argv[0]); 

    // set SIGINT handler function
    sethandler(SIGINT_handler, SIGINT);
    SIGINT_received = 0;

    // set global seed
    srand(1);
    
    // open the, TEMP_FAILURE_RETRY is used to prevent
    // problems with interrupting signals
    int filedes;
    if ((filedes = TEMP_FAILURE_RETRY(open(workfile_path, O_RDWR))) == -1)
        error("Cannot open the file");

    // obtain block size (-1 stands for \n)
    int block_size = (getfilelength(filedes) - 1) / blocks_count;

    // buffers
    struct aiocb aiocbs[BUFFERS_COUNT];
    char *buffers[BUFFERS_COUNT];

    if (block_size > 0)
    {
        create_buffers_and_fillaiostructs(buffers, block_size, aiocbs, filedes);
        processblocks(aiocbs, buffers, blocks_count, block_size, iteraions_count);
        cleanup(buffers, filedes);
    }

    // close the file, TEMP_FAILURE_RETRY is used to prevent
    // problems with interrupting signals
    if (TEMP_FAILURE_RETRY(close(filedes)) == -1)
        error("Cannot close the file");

    return EXIT_SUCCESS;
}

void SIGINT_handler(int sig)
{
	SIGINT_received = 1;
}

void choose_blocks(int *block1, int *block2, int max)
{
    *block1 = rand() % max;
    *block2 = rand() % max - 1;
    if (*block1 == *block2)
        (*block2)++;
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

void create_buffers_and_fillaiostructs(char* buffers[BUFFERS_COUNT], int block_size,
    struct aiocb* aiocbs, int filedes)
{
    // create buffers
    for (int i = 0; i < BUFFERS_COUNT; i++)
    {
        // calloc as malloc allocates memory but also
        // initializes it with zeros
        // it has different arguments - number of cells and size of cell
        buffers[i] = (char*)calloc(block_size, sizeof(char));
        if (buffers[i] == NULL)
            error("Buffer calloc failed");
    }

    // fill aio structs
    for (int i = 0; i < BUFFERS_COUNT; i++)
    {
        // prepare structure
        memset(&aiocbs[i], 0, sizeof(struct aiocb));
        aiocbs[i].aio_fildes = filedes;
        aiocbs[i].aio_offset = 0;
        aiocbs[i].aio_nbytes = block_size;
        aiocbs[i].aio_buf = (void *)buffers[i];
        aiocbs[i].aio_sigevent.sigev_notify = SIGEV_NONE;
    }
}

void cleanup(char **buffers, int filedes)
{
    if (SIGINT_received)
    {
        if (aio_cancel(filedes, NULL) == -1)
			error("Cannot cancel async. I/O operations");
    }

    for (int i = 0; i < BUFFERS_COUNT; i++)
		free(buffers[i]);

    if (TEMP_FAILURE_RETRY(fsync(filedes)) == -1)
		error("Error running fsync");
}

void reverse_buffer(char* buffer, int size)
{
    for (int i = 0; i < size / 2; i++)
    {
        char temp = buffer[i];
        buffer[i] = buffer[size - i - 1];
        buffer[size - i - 1] = temp;
    }
}

void processblocks(struct aiocb *aiocbs, char *buffers[BUFFERS_COUNT], 
    int blocks_count, int block_size, int iterations_count)
{
    int pos = 0;
    struct b {
        int ind1, ind2;
    } indexes[BUFFERS_COUNT];

    choose_blocks(&indexes[SHIFT(pos, 2)].ind1, 
        &indexes[SHIFT(pos, 2)].ind2, blocks_count);

    readdata(&aiocbs[SHIFT(pos, 2)], indexes[SHIFT(pos, 2)].ind1);
    suspend(&aiocbs[SHIFT(pos, 2)]);
    
    for (int i = 0; i < iterations_count - 1; i++)
    {
        if (i > 0)
            writedata(&aiocbs[pos], indexes[pos].ind2);

        if (i < iterations_count - 2)
        {
            choose_blocks(&indexes[SHIFT(pos, 1)].ind1, 
                &indexes[SHIFT(pos, 1)].ind2, blocks_count);
            readdata(&aiocbs[SHIFT(pos, 1)], indexes[SHIFT(pos, 1)].ind1);
        }

        reverse_buffer(buffers[SHIFT(pos, 2)], block_size);

        if (i > 0)
            syncdata(&aiocbs[pos]);

        if (i < iterations_count - 1)
            suspend(&aiocbs[SHIFT(pos, 1)]);
        
        pos = SHIFT(pos, 1);
    }
}
