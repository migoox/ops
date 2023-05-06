#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#define MSG_SIZE (PIPE_BUF - sizeof(pid_t))
#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s fifo_file file\n", name);
	exit(EXIT_FAILURE);
}

void write_to_fifo(int fifo_fd, int file_fd)
{
    int64_t real_size;
    char buffer[PIPE_BUF];

    // set the header of the buffer to current process's pid
    ((pid_t*)buffer)[0] = getpid();

    // save the data position in the buffer 
    char* buffer_data_pos = buffer + sizeof(pid_t);

    do {
        // read the data from the file
        if ((real_size = read(file_fd, buffer_data_pos, MSG_SIZE)) < 0)
            ERR("client: read file failed");
        
        // if it's less than MSG_SIZE fill the rest of the buffer with zeros
        if (real_size < MSG_SIZE)
            memset(buffer_data_pos + real_size, 0, MSG_SIZE - real_size);

        // pass the buffer to the fifo
        if (real_size > 0)
        {
            if (write(fifo_fd, buffer, PIPE_BUF) < 0)
                ERR("client: write fifo failed");
        }

    } while(real_size == MSG_SIZE);
}

int main(int argc, char **argv)
{
    int fifo_fd, file_fd;

    if (argc != 3)
        usage(argv[0]);

    if (mkfifo(argv[1], S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) < 0)
		if (errno != EEXIST)
			ERR("create fifo");
    
    if ((fifo_fd = open(argv[1], O_WRONLY)) < 0)
        ERR("client: open fifo failed");

    if ((file_fd = open(argv[2], O_RDONLY)) < 0)
        ERR("client: open file failed");

    write_to_fifo(fifo_fd, file_fd);

    if (close(fifo_fd) < 0)
        ERR("client: close fifo failed");

    if (close(file_fd) < 0)
        ERR("client: close file failed");

    return EXIT_SUCCESS;
}