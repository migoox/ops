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

#define CHILD_SLEEP_MIN 5
#define CHILD_SLEEP_MAX 10

// volatile tells compiler that the value can be modified from the
// different place than the code.
// It is not shared between processes!
volatile sig_atomic_t last_signal = 0; // typedef int

void update_last_signal_handler(int signal)
{
    printf("Process with pid %d, received a signal %d.\n", getpid(), signal);
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

    // an alternative way:
    //      sigemptyset(&sa.sa_mask);
    //      sa.sa_flags = 0;

    // setting handler 
    sa.sa_handler = handler;

    if (sigaction(signal, &sa, NULL) == -1)
    {
        ERR("sigaction() failed");
    }
}

void child_work(int r)
{
    int seconds = CHILD_SLEEP_MIN + 
            rand() % (CHILD_SLEEP_MAX - CHILD_SLEEP_MIN + 1);

    for (int i = 0; i < r; i++)
    {
        int seconds_to_sleep = seconds;

        // sleeping must happen inside of the loop since
        // signal may interrupt it
        while ((seconds_to_sleep = sleep(seconds_to_sleep)) > 0)
            ;

        if (last_signal == SIGUSR1)
        {
            printf("SUCCESS, pid: %d.\n", getpid());
        }
        else // last_signal == SIGUSR2
        {
            printf("FAILURE, pid: %d.\n", getpid());
        }
    }

    printf("A child with pid %d has been terminated.\n", getpid());
}

void create_children(int n, int r)
{
    for (int i = 0; i < n; i++)
    {
        pid_t pid = fork();

        if (pid == 0)
        {
            // set handlers for SIGUSR1 and SIGUSR2
            set_handler(SIGUSR1, update_last_signal_handler);
            set_handler(SIGUSR2, update_last_signal_handler);

            // generate unique seed for each child
            srand(time(NULL) * getpid());

            child_work(r);

            exit(EXIT_SUCCESS);
        } 

        if (pid == -1)
        {
            ERR("Fork failed");
        }
    }
}

void parent_work(int k, int p, int r)
{
    struct timespec time1 = {k, 0}; 
    struct timespec time2 = {p, 0}; 

    alarm(CHILD_SLEEP_MAX * r);

    while (last_signal != SIGALRM)
    {
        nanosleep(&time1, NULL);

        // 0 means all processes in the group
        kill(0, SIGUSR1);

        nanosleep(&time2, NULL);

        kill(0, SIGUSR2);
    }
}

void usage(const char *prog_name)
{
   	fprintf(stderr, "USAGE: %s n k p r\n", prog_name);
	fprintf(stderr, "n - number of children\n");
	fprintf(stderr, "k - Interval before SIGUSR1\n");
	fprintf(stderr, "p - Interval before SIGUSR2\n");
	fprintf(stderr, "r - lifetime of child in cycles\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	int n, k, p, r;

	if (argc != 5)
		usage(argv[0]);

	n = atoi(argv[1]);
	k = atoi(argv[2]);
	p = atoi(argv[3]);
	r = atoi(argv[4]);

	if (n <= 0 || k <= 0 || p <= 0 || r <= 0)
		usage(argv[0]);

    // Sending using kill with pid 0, sends signal also to a sender
    set_handler(SIGUSR1, SIG_IGN);
    set_handler(SIGUSR2, SIG_IGN);

    // Setting parent to update last signal if it's SIGALRM
    set_handler(SIGALRM, update_last_signal_handler);

    // SIGCHLD is sent to the parent process by the system
    // if one of the children is terminated
    set_handler(SIGCHLD, child_terminated_signal_handler);

    create_children(n, r);
    parent_work(k, p, r);


    // Wyliczenie czasu w pętli rodzica nie wystarczy, 
    // w obciążonym systemie możliwe są dowolnie długie opóźnienia, bez wait
    // powstaje zatem tzw. race condition - kto się pierwszy zakończy 
    // rodzic czy potomne procesy. Zatem musimy użyc dla pewnosci wait 
    while (wait(NULL) > 0)
		;

    return EXIT_SUCCESS;
}