#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PATH_BUFFER_MAX_SIZE 128
#define ERR(source)                                                                                                    \
	(fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), \
    perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t signal_count = 0;

void increment_signal_count(int signal)
{
    signal_count++;
}

void set_handler(int signal, void (*handler)(int))
{
    struct sigaction sa;

    // initalizing all of the members with zero
    memset(&sa, 0, sizeof(struct sigaction));

    // setting handler 
    sa.sa_handler = handler;

    if (sigaction(signal, &sa, NULL) == -1)
    {
        ERR("sigaction() failed");
    }
}

void child_work(int m)
{
    const struct timespec time = {0, m * 1000};

    while (1)
    {
        nanosleep(&time, NULL);
        if (kill(getppid(), SIGUSR1))
            ERR("kill() failed");
    }
}

ssize_t bulk_read(int fd, char *buf, size_t count)
{
	ssize_t c;
	ssize_t len = 0;
	do {
        // c means how many bytes were read
		c = TEMP_FAILURE_RETRY(read(fd, buf, count));

        // failure
		if (c < 0)
			return c;

        // there is nothing to read (EOF)
		if (c == 0)
			return len;
        
        // move buffer (pointer is a copy)
		buf += c;

		len += c;
		count -= c;

	} while (count > 0);

	return len;
}

ssize_t bulk_write(int fd, char *buf, size_t count)
{
	ssize_t c;
	ssize_t len = 0;
	do {
        // c means how many bytes were written
		c = TEMP_FAILURE_RETRY(write(fd, buf, count));

        // failure
		if (c < 0)
			return c;
        
        // move buffer (pointer is a copy)
		buf += c;

		len += c;
		count -= c;

	} while (count > 0);

	return len;
}


void create_child(int m)
{
    pid_t pid = fork();
    if (pid == 0)
    {
        set_handler(SIGUSR1, SIG_DFL);
        child_work(m);
        exit(EXIT_SUCCESS);
    }

    if (pid == -1)
    {
        ERR("fork() failed");
    }
}

void parent_work(int blocks_num, int size, const char* file_name)
{
    // open/create the file with the given name
    int file_fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0777);
    
    if (file_fd == -1) ERR("open() failed");

    // open urandom file
    int rand_fd = open("/dev/urandom", O_RDONLY);

    if (rand_fd == -1) ERR("open() failed");

    // prepare a buffer
    char* buffer = (char*) malloc(size);
    if (buffer == NULL) ERR("malloc() failed");

    long int real_size = 0;

    for (int i = 0; i < blocks_num; i++)
    {
        if ((real_size = bulk_read(rand_fd, buffer, size)) < 0 ) ERR("read() failed");

        if ((real_size = bulk_write(file_fd, buffer, real_size)) < 0) ERR("read() failed");

        if (TEMP_FAILURE_RETRY(fprintf(stderr, "Block of %ld bytes transfered. Signals RX:%d\n", real_size, signal_count)) < 0)
            ERR("fprintf() failed");
    }
    free(buffer);

    if (TEMP_FAILURE_RETRY(close(rand_fd)) == -1) ERR("close() failed");

    if (TEMP_FAILURE_RETRY(close(file_fd)) == -1) ERR("close() failed");

    if (kill(0, SIGUSR1) == -1) ERR("kill() failed");
}

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s m b s \n", name);
	fprintf(stderr, "m - number of 1/1000 milliseconds between signals [1,999], "
			"i.e. one milisecond maximum\n");
	fprintf(stderr, "b - number of blocks [1,999]\n");
	fprintf(stderr, "s - size of of blocks [1,999] in MB\n");
	fprintf(stderr, "name of the output file\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    int m, b, s;
	char *name;

	if (argc != 5)
		usage(argv[0]);
	m = atoi(argv[1]);
	b = atoi(argv[2]);
	s = atoi(argv[3]);
	name = argv[4];

	if (m <= 0 || m > 999 || b <= 0 || b > 999 || s <= 0 || s > 999)
		usage(argv[0]);

    set_handler(SIGUSR1, increment_signal_count);
    
    create_child(m);
    parent_work(b, s * 1024 * 1024, name);

    while (wait(NULL) > 0)
		;

    return EXIT_SUCCESS;
}