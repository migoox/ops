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

#define INTERVAL_MIN_KB 10
#define INTERVAL_MAX_KB 100 

#define MAX_PID_LENGTH 20

#define ERR(source) \
	(fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), \
     perror(source), kill(0, SIGKILL), \
     exit(EXIT_FAILURE))


void signal_handler(int signal)
{

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

void child_work(int i, int c, int s, int t)
{

}

void usage(const char *prog_name)
{
    printf("Usage: %s ...params\n", prog_name);
}

void create_children(int children_count, char** children_params)
{

}

int main(int argc, char** argv)
{
    char** children_params = argv + 1;
    int children_count = argc - 1;

    if (children_count > 8 || children_count < 2)
        usage(argv[0]);

    for (int i = 0; i < children_count; i++)
    {
        if (atoi(children_params[i]) > 2 || atoi(children_params[i]) < 1)
            usage(argv[0]);
    }

    create_children(children_count, children_params);


    // loop responsible for wating children
    while (children_count > 0)
    {
        while (1)
        {
            errno = 0;
            int status;
            pid_t pid = waitpid(0, &status, WNOHANG);

            if (pid > 0)
            {
                children_count--;
            }

            else if (pid == 0)
            {
                break;
            }
            else // pid < 0
            {

                if (errno == ECHILD)
                    break;
                
                // in this place of the program we can be sure that there is an error
                ERR("waitpid() error");            
            }
        }
    }

    return EXIT_SUCCESS;
}