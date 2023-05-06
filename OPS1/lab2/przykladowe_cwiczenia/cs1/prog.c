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

volatile int signal_count1 = 0;
volatile int signal_count2 = 0;

void parent_signal_handler(int signal)
{
    if (signal == SIGUSR1)
        signal_count1++;
    
    if (signal == SIGUSR2)
        signal_count2++;

    printf("[PARENT, pid: %d] *\n", getpid());
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
    printf("[CHILD, pid: %d] i = %d, c = %d, s = %d, t = %d\n", getpid(), i, c, s, t);

    struct timespec time = {0, t * 1000 * 1000};

    s = (s == 1 ? SIGUSR1 : SIGUSR2);

    for (int i = 0; i < c; i++)
    {
        if (kill(getppid(), s) == -1) ERR("kill() failed");

        nanosleep(&time, NULL);
    }
}

void usage(const char *prog_name)
{
    printf("Usage: %s ...params\n", prog_name);
}

void bin(unsigned int n)
{
    for (int i = 1 << 30; i > 0; i = i / 2)
    {
        if((n & i) != 0)
        {
            printf("1");
        }
        else
        {
            printf("0");
        }
    }
}

void create_children(int children_count, char** children_params)
{
    for (int i = 1; i <= children_count; i++)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            srand(time(NULL) * getpid());

            int c = 1 + rand() % (i - 1 + 1);
            int s = atoi(children_params[i - 1]);
            int t = 10 + rand() % (30 - 10 + 1);

            child_work(i, c, s, t);

            // Use bitwise OR operator (|) to combine the two integers
            int status = c | (s << 4);

            exit(status);
        }

        if (pid < 0)
        {
            ERR("fork() failed");
        }
    }
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

    set_handler(SIGUSR1, parent_signal_handler);
    set_handler(SIGUSR2, parent_signal_handler);

    create_children(children_count, children_params);

    int sum1 = 0;
    int sum2 = 0;

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
                printf("[PARENT, pid: %d] child with pid %d has been waited.\n", getpid(), pid);
                int ev_status = WEXITSTATUS(status);
               
                int c = ev_status & 0xf;
                int s = (ev_status >> 4) & 0xf;
                if (s == 1)
                {
                    sum1 += c;
                }
                else
                {
                    sum2 += c;
                }
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
    
    printf("[PARENT, pid: %d] Sum1 = %d, Sum2 = %d, sig1 = %d, sig2 = %d .\n", 
        getpid(), sum1, sum2, signal_count1, signal_count2);

    return EXIT_SUCCESS;
}