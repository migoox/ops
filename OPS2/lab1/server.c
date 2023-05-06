#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s fifo_file\n", name);
	exit(EXIT_FAILURE);
}

void read_from_fifo(int fifo_fd)
{
    // singed size_t
    int64_t real_size;
    char buffer[PIPE_BUF];

    do {
        // at the first read there will be block, till 
        // the other process will connect to the fifo and 
        // and will provide some data
        if ((real_size = read(fifo_fd, buffer, PIPE_BUF)) < 0)
            ERR("read error");

        if (real_size > 0)
        {
            pid_t sender_pid = *((pid_t*)buffer);

            printf("\n===================================\n");
            printf("Sender pid: %d\n", sender_pid);
            printf("Message: \n\"");
            for (char* c = buffer + sizeof(pid_t); c != buffer + PIPE_BUF; ++c)
            {
                if (isalnum(*c))
                    printf("%c", *c);
            }
            printf("\"\n===================================\n");
        }

    // if count == 0, that means that FIFO has ended (EOF)
    // this will happen if all of the writers disconnect
    // and the fifo has no data
    } while(real_size > 0);
}   

int main(int argc, char** argv)
{
    int fifo_fd;

    // the path to the fifo file should be provided as an argument
    if (argc != 2)
        usage(argv[0]);

    // make fifo with read ride for users and groups
    if (mkfifo(argv[1], S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) < 0)
        if (errno != EEXIST) // if fifo was created we don't treat that as an error
            ERR("mkfifo error");

    // as the server is a reader, it should open the fifo with O_RDONLY
    if ((fifo_fd = open(argv[1], O_RDONLY)) < 0)
        ERR("fifo open error");

    read_from_fifo(fifo_fd);

    if (close(fifo_fd) < 0)
        ERR("fifo close error");
    
    // delete FIFO
    if (unlink(argv[1]) < 0)
		ERR("remove fifo:");

    return EXIT_SUCCESS;
}