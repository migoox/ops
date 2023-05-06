#define _GNU_SOURCE
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))
#define MAX_SERIES_COUNT 10

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s fifo_file, factor, elements...\n", name);
	exit(EXIT_FAILURE);
}

void create_children(const char* fifo_path, int series_count, int* series_elems);
void child_work(int elem, int prnt_pipe_in, int id);

void wait_children_with_suspend();

int main(int argc, char** argv)
{
    if (argc < 3)
        usage(argv[0]);
    if (argc > MAX_SERIES_COUNT + 3)
        usage(argv[0]);

    const char *fifo_path = argv[1];
    int factor = atoi(argv[2]);
    int series_elems[MAX_SERIES_COUNT];
    int series_count = argc - 3;
    for(int i = 0; i < series_count; ++i) {
        series_elems[i] = atoi(argv[i + 3]);
    }

    // check input
    // printf("fifo: %s\n", fifo_path);
    // printf("factor: %d\n", factor);
    // for(int i = 0; i < series_count; ++i) {
    //     printf("elem %d: %d\n", i, series_elems[i]);
    // }

    // create FIFO
    if (mkfifo(fifo_path, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP) < 0)
        if (errno != EEXIST) // if fifo was created we don't treat that as an error
            ERR("mkfifo error");

    create_children(fifo_path, series_count, series_elems);

    int pipe_out;
    if ((pipe_out = TEMP_FAILURE_RETRY(open(fifo_path, O_RDONLY))) < 0) {
        ERR("(parent) open failed");
    }


    wait_children_with_suspend();
    if (TEMP_FAILURE_RETRY(close(pipe_out)) < 0) {
        ERR("(parent) open failed");
    }

    // delete FIFO
    if (unlink(fifo_path) < 0)
		ERR("remove fifo:");
    return 0;
}

void create_children(const char* fifo_path, int series_count, int* series_elems)
{
    for (int i = 0; i < series_count; ++i) {
        // fork
        pid_t child_id = fork();
        if (child_id == 0) {
            
            int fd;
            if ((fd = TEMP_FAILURE_RETRY(open(fifo_path, O_WRONLY))) < 0) {
                ERR("(child) open failed");
            }

            child_work(series_elems[i], fd, i);

            if (TEMP_FAILURE_RETRY(close(fd)) < 0) {
                ERR("(child) open failed");
            }
            // exit 
            exit(EXIT_SUCCESS);

        } else if (child_id < 0) {
            ERR("fork failed");
        }

    }
}

void child_work(int elem, int prnt_pipe_in, int id)
{
    printf("child, id: %d, pid: %d, elem: %d\n", id, getpid(), elem);
}

void wait_children_with_suspend()
{
    for (;;) {
        pid_t chld_pid = wait(NULL);
        if (-1 == chld_pid && ECHILD == errno) { // no children to wait for
            break;
        } else if (-1 == chld_pid) { // error
            ERR("waiting failed"); 
        }
    }
}
