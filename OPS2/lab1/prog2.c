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

// MAX_BUFF must be in one byte range - [0, 255]
#define MAX_BUFF 200

volatile sig_atomic_t last_signal = 0;

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s n\n", name);
	fprintf(stderr, "0<n<=10 - number of children\n");
	exit(EXIT_FAILURE);
}

void sig_handler(int sig)
{
	last_signal = sig;
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

void chld_kill_handler(int sigNo)
{
    // there is 20% chance that the child will be killed
    if (rand() % 5 == 0)
    {
        printf("child with pid %d has been killed\n", getpid());
        exit(EXIT_SUCCESS);
    }
}

void sigchld_handler(int sigNo)
{
    pid_t chld_pid;
    // we use loop, since there might be more than one child to be waited
    for (;;) {
        // we use WNOHANG option, so the parent won't be suspended
        // if there are no children to wait for
        // every child inherited its process group from parent
        // so using 0 as pid will work
        chld_pid = waitpid(0, NULL, WNOHANG);

        if (chld_pid == 0) { // no children statuses available
            return;
        } else if (-1 == chld_pid && ECHILD == errno) { // no children exist
            return;
        } else if (-1 == chld_pid){ // error
            ERR("waiting failed"); 
        }
    }
}

// pipe_R_out and pipes_P_in are out parameters
void create_children_and_pipes(int count, int *pipe_R_out, int **pipes_P_in);
void child_work_func(int pipe_in, int pipe_out);
void parent_work_func(int children_count, int pipe_out, int *pipes_in);

int main(int argc, char **argv)
{
    // create a seed
    srand(time(NULL));

    if (argc != 2)
        usage(argv[0]);

    // get children count
    int children_count = atoi(argv[1]);
    if (children_count <= 0 || children_count > 10)
        usage(argv[0]);
    
    // set SIGCHLD handler
    // SIGCHLD is sent by a child if it's terminated
    if (set_handler(sigchld_handler, SIGCHLD))
        ERR("Setting parent SIGCHLD handler failed:");

    // set SIGINT handler
    if (set_handler(sig_handler, SIGINT))
        ERR("Setting parent SIGINT handler failed:");

    // ignore SIGPIPE (if it is not ignored the program would be terminated because of parent_work)
	if (set_handler(SIG_IGN, SIGPIPE))
		ERR("Setting parent SIGPIPE handler failed:");

    // pipes containers
    int pipe_out;
    int *pipes_in;

    create_children_and_pipes(children_count, &pipe_out, &pipes_in);
    parent_work_func(children_count, pipe_out, pipes_in);

    // close file descriptors (pipes_in is already closed in this point of the program)

    if (close(pipe_out) < 0)
        ERR("(parent) close failed");

    // free the array of pipes
    free(pipes_in);

    return EXIT_SUCCESS;
}

// out - read = 0, in - write = 1 
void create_children_and_pipes(int count, int *pipe_R_out, int **pipes_P_in)
{
    // create the R pipe
    int pipe_R[2];
    if (pipe(pipe_R) < 0) 
        ERR("pipe creation failed");

    // prepare 
    int *pipes_P = (int*)malloc(2 * count * sizeof(int)); 
    if (pipes_P == NULL)
        ERR("malloc failed");

    for (int i = 0; i < count; ++i) {
        // create a pipe
        if (pipe(&pipes_P[i * 2]) < 0)
            ERR("pipe P creation failed");

        // fork a child
        pid_t result = fork();
        if (result == 0) { // child process
            // init rand
            srand(time(NULL) * getpid());
            // close all the pipes inherited except the one created in the current iteration
            for (int j = 0; j < i; ++j) {
                if (TEMP_FAILURE_RETRY(close(pipes_P[j * 2])) < 0 
                    || TEMP_FAILURE_RETRY(close(pipes_P[j * 2 + 1])) < 0)
                    ERR("(child) pipe P closing failed");
            }

            // close pipe P write fd
            if (TEMP_FAILURE_RETRY(close(pipes_P[i * 2 + 1])) < 0)
                ERR("(child) pipe P closing failed");

            // close pipe R read fd
            if (TEMP_FAILURE_RETRY(close(pipe_R[0])) < 0)
                ERR("(child) pipe R closing failed");

            int pipe_in = pipe_R[1];
            int pipe_out = pipes_P[i * 2];

            // free memory allocated by the parent
            free(pipes_P);

            // set handler
            if (set_handler(chld_kill_handler, SIGINT))
                ERR("(child) set handler failed");

            // start working
            child_work_func(pipe_in, pipe_out);

            // close the pipes
            if (TEMP_FAILURE_RETRY(close(pipe_in)) < 0 
                || TEMP_FAILURE_RETRY(close(pipe_out < 0)))
                ERR("(child) pipe close failed");

            // end the life
            exit(EXIT_SUCCESS);
        } else if (result < 0) { // error
            ERR("fork failed");
        }
    }

    // close R's write fd
    if (TEMP_FAILURE_RETRY(close(pipe_R[1])))
        ERR("(parent) pipe R close failed");

    // save R's read fd
    *pipe_R_out = pipe_R[0];

    // allocate memory for the outpu variable
    *pipes_P_in = (int*)malloc(count * sizeof(int));
    if (*pipes_P_in == NULL)
        ERR("malloc failed");

    // close read pipes_P and save write pipes_P
    for (int i = 0; i < count; ++i) 
    {
        if (close(TEMP_FAILURE_RETRY(pipes_P[i * 2])))
            ERR("(parent) pipe P closing failed");

        (*pipes_P_in)[i] = pipes_P[i * 2 + 1];
    }

    free(pipes_P);
}

void child_work_func(int pipe_in, int pipe_out)
{
    char c, buffer[MAX_BUFF + 1];
    unsigned char s;

    for (;;) {
        if (TEMP_FAILURE_RETRY(read(pipe_out, &c, 1)) < 1)
			ERR("(child) read failed");

        int max = MAX_BUFF > PIPE_BUF ? PIPE_BUF : MAX_BUFF;
        s = 1 + rand() % max;
        buffer[0] = s;
        memset(buffer + 1, c, s);
        if (TEMP_FAILURE_RETRY(write(pipe_in, buffer, s + 1)) < 0)
            ERR("(child) write failed");
    }
}

void parent_work_func(int children_count, int pipe_out, int *pipes_in)
{
    char buffer[MAX_BUFF];
    for (;;) {

        // writing
        if (SIGINT == last_signal) {
            // reset the last_signal flag
            last_signal = 0;

            // chose random child process (there is probability that this child is dead)
            int child_id = rand() % children_count;

            // round robin till the child is not dead
            while (0 == pipes_in[child_id])
                child_id = (child_id + 1) % children_count;

            if (pipes_in[child_id]) {
                unsigned char c = 'a' + rand() % ('z' - 'a');
                int status = TEMP_FAILURE_RETRY(write(pipes_in[child_id], &c, 1));

                // if there is no reader, -1 will be returned, SIGPIPE will be sent and errno=EPIPE 
                // as SIGPIPE is a Term signal, it should be handled (e.g. ignored) by the program
                if (status != 1) {
                    // the pipe should be closed
					if (TEMP_FAILURE_RETRY(close(pipes_in[child_id])))
						ERR("(parent) close");
                    
                    // set fd to 0
					pipes_in[child_id] = 0;
				}
            }
        }

        // read size of the buffer
        unsigned char size;
        // by default the parent waits on blocked read for SIGINT
        // if SIGINT is delivered, this read is interrupted
        // and the loop continues 
        int status = read(pipe_out, &size, 1);
        if (status < 0 && EINTR == errno) 
            continue;
        if (status < 0) // error during reading
            ERR("(parent) read failed");
        if (0 == status) // EOF
            break;
        
        // read buffer
        if (TEMP_FAILURE_RETRY(read(pipe_out, buffer, size)) < size)
            ERR("(parent) read data from R");

        // set termination sign
        buffer[(int)size] = 0;
        printf("\n%s\n", buffer);
    }   
}