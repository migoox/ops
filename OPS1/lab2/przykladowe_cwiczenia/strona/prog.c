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

volatile int signal_count = 0;
volatile int alarm_received = 0;

void child_signal_handler(int signal)
{
    if (signal == SIGALRM)
    {
        alarm_received = 1;
    }
    ++signal_count;
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

void usage(const char *prog_name)
{
    printf("Usage: %s ...params\n", prog_name);
}

ssize_t bulk_write(int fd, char *buf, size_t count)
{
	ssize_t c;
	ssize_t len = 0;
	do {
		c = TEMP_FAILURE_RETRY(write(fd, buf, count));
		if (c < 0)
			return c;
		buf += c;
		len += c;
		count -= c;
	} while (count > 0);
	return len;
}

void child_work(int n, int s, sigset_t oldmask)
{
    char* buffer = (char *)malloc(s);
    if (buffer == NULL) ERR("malloc() failed");

    // 5 stands for .txt\0
    char file_name[MAX_PID_LENGTH + 5];
    sprintf(file_name, "%d", getpid());
    strcat(file_name, ".txt");

    int fd = TEMP_FAILURE_RETRY(open(file_name, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0777));

    if (fd == -1) ERR("open() failed");

    int count = 0;

    for (int i = 0; i < s; i++)
    {
        buffer[i] = n + '0';
    }

    alarm(1);
    while (alarm_received == 0) 
    {
		while (signal_count == count)
			sigsuspend(&oldmask);
        
        for (int i = 0; i < signal_count - count; i++)
        {   
            if ((bulk_write(fd, buffer, s)) < 0) ERR("write() failed");
        }
        count = signal_count;
	}

    if (TEMP_FAILURE_RETRY(close(fd)) == -1) ERR("close() failed");

    free(buffer);
}

void create_children(int children_count, char** children_params)
{
    for (int i = 0; i < children_count; i++)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            // child 
            set_handler(SIGUSR1, child_signal_handler);
            set_handler(SIGALRM, child_signal_handler);

            sigset_t mask, oldmask;
            sigemptyset(&mask);
            sigaddset(&mask, SIGUSR1);
            sigprocmask(SIG_BLOCK, &mask, &oldmask);

            srand(time(NULL) * getpid());

            int n = atoi(children_params[i]);
            int s = INTERVAL_MIN_KB + rand() % (INTERVAL_MAX_KB - INTERVAL_MIN_KB + 1);

            child_work(n, s * 1024, oldmask);

            exit(EXIT_SUCCESS);
        }

        if (pid < 0)
        {
            ERR("fork() failed");
        }
    }
}

void parent_work()
{
    struct timespec time = {0, 1000 * 1000 * 10};
    struct timespec unslept = {0, 0};

    for (int i = 0; i < 100; i++)
    {
        if (kill(0, SIGUSR1) == -1) ERR("kill() failed");

        // nanosleep may be interrupted
        struct timespec temp = time;
        while (temp.tv_nsec > 0 || temp.tv_sec > 0)
        {
            nanosleep(&temp, &unslept);
            temp = unslept;
        }
    }
}

int main(int argc, char** argv)
{
    char** children_params = argv + 1;
    int children_count = argc - 1;

    for (int i = 0; i < children_count; i++)
    {
        if (atoi(children_params[i]) > 9 || atoi(children_params[i]) < 0)
            usage(argv[0]);
    }

    set_handler(SIGUSR1, SIG_IGN);
    create_children(children_count, children_params);
    parent_work();

    pid_t pid;
    while ((pid = wait(NULL)) > 0)
    {
        printf("[PARENT, pid: %d] child with pid %d has been waited.\n", getpid(), pid);
    }

    return EXIT_SUCCESS;
}