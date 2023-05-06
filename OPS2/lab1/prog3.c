#include <asm-generic/errno-base.h>
#include <linux/limits.h>
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

#define ERR(source)                                                                                                    \
	(fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s\n", name);
	exit(EXIT_FAILURE);
}


int set_handler(void (*f)(int), int sigNo)
{
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1 == sigaction(sigNo, &act, NULL))
		return -1;
	return 0;
}

void create_children_and_pipes(int* pipe_in, int* pipe_out);
void work(int pipe_in, int pipe_out, int id);

int main(int argc, char **argv)
{   
    if (argc != 1)
        usage(argv[0]);

    int pipe_in, pipe_out;

    // pipe_in and pipe_out are both output of this function
    create_children_and_pipes(&pipe_in, &pipe_out);
    
    // start working
    work(pipe_in, pipe_out, 0);

    // ignore errno=EPIPE
    if (set_handler(SIG_IGN, SIGPIPE) < 0)
		ERR("Setting SIGPIPE handler failed:");

    // close parent pipes
    if (TEMP_FAILURE_RETRY(close(pipe_in))
        || TEMP_FAILURE_RETRY(close(pipe_out))) {
            ERR("child 2 closing failed");
        }

    // wait for children
    for (;;) {
        pid_t chld_pid = wait(NULL);

        if (-1 == chld_pid && ECHILD == errno) { // no children to wait for
            break;
        } else if (-1 == chld_pid) { // error
            ERR("waiting failed"); 
        }
    }

    return EXIT_SUCCESS;
}

void create_children_and_pipes(int* pipe_in, int* pipe_out)
{
    // parent - A[1]--A-->A[0] - child1 - B[1]--B-->B[0] - child2 - C[1]--C-->C[0] - parent

    // create pipes
    int A[2], B[2], C[2];
    if (pipe(A) < 0) ERR("pipe A creation failed");
    if (pipe(B) < 0) ERR("pipe B creation failed");
    if (pipe(C) < 0) ERR("pipe C creation failed");

    // printf("pipe A: %d-->%d\n", A[1], A[0]);
    // printf("pipe B: %d-->%d\n", B[1], B[0]);
    // printf("pipe C: %d-->%d\n", C[1], C[0]);

    // fork child 1
    pid_t child_pid = fork();
    if (0 == child_pid) {
        // close everything except A[0] and B[1]
        if (TEMP_FAILURE_RETRY(close(A[1]))
            || TEMP_FAILURE_RETRY(close(B[0]))
            || TEMP_FAILURE_RETRY(close(C[0]))
            || TEMP_FAILURE_RETRY(close(C[1]))) {
                ERR("child 1 closing failed");
            }

        work(B[1], A[0], 1);

        if (TEMP_FAILURE_RETRY(close(B[1]))
            || TEMP_FAILURE_RETRY(close(A[0]))) {
                ERR("child 2 closing failed");
            }

        exit(EXIT_SUCCESS);
    } else if (child_pid < 0) {
        ERR("fork failed");
    }

    // fork child 2
    child_pid = fork();
    if (0 == child_pid) {
        // close everything except B[0] and C[1]
        if (TEMP_FAILURE_RETRY(close(A[0]))
            || TEMP_FAILURE_RETRY(close(A[1]))
            || TEMP_FAILURE_RETRY(close(B[1]))
            || TEMP_FAILURE_RETRY(close(C[0]))) {
                ERR("child 2 closing failed");
            }

        work(C[1], B[0], 2);

        if (TEMP_FAILURE_RETRY(close(C[1]))
            || TEMP_FAILURE_RETRY(close(B[0]))) {
                ERR("child 2 closing failed");
            }

        exit(EXIT_SUCCESS);
    } else if (child_pid < 0) {
        ERR("fork failed");
    }

    // (parent) close everything except A[1] and C[0]
    if (TEMP_FAILURE_RETRY(close(A[0]))
        || TEMP_FAILURE_RETRY(close(B[0]))
        || TEMP_FAILURE_RETRY(close(B[1]))
        || TEMP_FAILURE_RETRY(close(C[1]))) {
            ERR("parent closing failed");
    }

    *pipe_in = A[1];
    *pipe_out = C[0];
}

void work(int pipe_in, int pipe_out, int id)
{
    // printf("id: %d, pipe_in: %d, pipe_out: %d\n", chld_id, pipe_in, pipe_out);

    srand(time(NULL) * getpid());

    int num = 1;

    // parent
    if (id == 0) {
        printf("parent sends\n");
        if (TEMP_FAILURE_RETRY(write(pipe_in, (void*)(&num), sizeof(int))) < 0)
            ERR("writing error");
    }

    for(;;) {

        int count = 0;
        if ((count = TEMP_FAILURE_RETRY(read(pipe_out, (void*)(&num), sizeof(int)))) < 0)
            ERR("reading error");

        if (count == 0) {// EOF (appears )
            printf("pid: %d - QUIT (EOF)\n", getpid());
            break;
        }
        printf("pid: %d, message received: %d \n", getpid(), num);

        // stop condition
        if (num == 0) {
            printf("pid: %d - QUIT\n", getpid());
            break;
        }
        num = num + (rand() % 21) - 10;

        if (TEMP_FAILURE_RETRY(write(pipe_in, (void*)(&num), sizeof(int))) < 0) {
            if (EPIPE != errno)
                ERR("writing error");
            
            // shouldn't happen
            printf("pid: %d - QUIT (EPIPE)\n", getpid());
            break;
        }

    }
}