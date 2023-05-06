#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

#define ERR(source)                                                                                                    \
	(fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), \
    perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

volatile sig_atomic_t last_signal = 0;
volatile int receiver_counter = 0;
volatile int SIGUSR2_received = 0; 


void update_last_signal_handler_receiver(int signal)
{
    if (signal == SIGUSR2)
    {
        receiver_counter++;
        SIGUSR2_received = 1;
    }
    last_signal = signal;
}

void child_terminated_signal_handler(int signal)
{
    while (1)
    {
        errno = 0;
        pid_t pid = waitpid(0, NULL, WNOHANG);

        if (pid > 0)
        {
            printf("Child with pid %d was waited.\n", pid);
        }

        // all of the terminated children where waited 
        if (pid == 0)
        {
            break;
        }

        if (pid == -1)
        {
            if (errno == ECHILD)
                break;
            
            ERR("waitpid() failed");
        }
    }
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

void child_work(int t, int n)
{
    const struct timespec time = {0, t * 1000};

    int i = 1;
    int counter = 0;
    while (1)
    {
        nanosleep(&time, NULL);
        if (n == i)
        {
            i = 1;

            printf("[CHILD] SIGUSR2 signal was sent %d times\n", ++counter);

            if (kill(getppid(), SIGUSR2))
                ERR("kill() failed");

            continue;
        }

        if (kill(getppid(), SIGUSR1))
            ERR("kill() failed");

        i++;
    }
}

void create_child(int t, int n)
{
    pid_t pid = fork();
    if (pid == 0)
    {
        child_work(t, n);
        exit(EXIT_SUCCESS);
    }

    if (pid == -1)
    {
        ERR("fork() failed");
    }
}

void parent_work(sigset_t oldmask)
{
    while(1)
    {
        SIGUSR2_received = 0;
        while (SIGUSR2_received == 0)
        {
            // only here SIGUSR1 and SIGUSR2 can be received
            sigsuspend(&oldmask);
        }
        printf("[PARENT] SIGUSR2 signal was received %d times\n", receiver_counter);
    }
}

void usage(const char *prog_name)
{
   	fprintf(stderr, "USAGE: %s t n\n", prog_name);
    fprintf(stderr, "t - time between attempts in microseconds\n");
	fprintf(stderr, "n - after how many attempts SIGUSR2 shall be sent\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	int t, n;

	if (argc != 3)
		usage(argv[0]);

	t = atoi(argv[1]);
	n = atoi(argv[2]);;

	if (t <= 0 || n <= 0)
		usage(argv[0]);

    set_handler(SIGCHLD, child_terminated_signal_handler);
    set_handler(SIGUSR1, update_last_signal_handler_receiver);
    set_handler(SIGUSR2, update_last_signal_handler_receiver);
    
    
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
	sigaddset(&mask, SIGUSR2);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    create_child(t, n);

    parent_work(oldmask);

    while (wait(NULL) > 0)
		;

    return EXIT_SUCCESS;
}