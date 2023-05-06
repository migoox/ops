#define _GNU_SOURCE
#include <asm-generic/errno-base.h>
#include <bits/types/sigevent_t.h>
#include <linux/limits.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <mqueue.h>
#include <stdint.h>

#define ERR(source)                                                                                                    \
	(fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define MAX_NUM 10
#define LIFE_SPAN 10

volatile sig_atomic_t children_left = 0;

void usage(void)
{
	fprintf(stderr, "USAGE: signals n k p l\n");
	fprintf(stderr, "100 >n > 0 - number of children\n");
	exit(EXIT_FAILURE);
}

void set_handler(void (*f)(int, siginfo_t *, void *), int sigNo)
{
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_sigaction = f;
    // this flag changes expected function handler
    // now apart form signal no, we receive info (structure with 
    // additional information about the event)
    // and context (stack pointer and execution context)
	act.sa_flags = SA_SIGINFO;
	if (-1 == sigaction(sigNo, &act, NULL))
		ERR("sigaction");
}

void sigchld_handler(int sig, siginfo_t *s, void *p);
void mq_handler(int sig, siginfo_t *info, void *p);
void create_children(int children_count, mqd_t pin, mqd_t pout);
void parent_work(mqd_t pout);
void child_work(int id, mqd_t pin, mqd_t pout);

int main(int argc, char **argv)
{
    if (argc != 2)
        usage();

    // get chidlren count from the arguments
    int children_count = atoi(argv[1]);
    if (children_count <= 0 || children_count >= 100)
        usage();

    // prepare queues attributes
    struct mq_attr attr;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = 1;

    // open both queues (in and out)
    mqd_t pin, pout;
    if ((pin = TEMP_FAILURE_RETRY(mq_open("/bingo_in", O_RDWR | O_NONBLOCK | O_CREAT, 0600, &attr))) == (mqd_t)-1) {
        ERR("mq_open for pin failed");
    }
    if ((pout = TEMP_FAILURE_RETRY(mq_open("/bingo_out", O_RDWR | O_CREAT, 0600, &attr))) == (mqd_t)-1) {
        ERR("mq_open for pin failed");
    }

    // set handlers
    set_handler(sigchld_handler, SIGCHLD);
    set_handler(mq_handler, SIGRTMIN);
    
    create_children(children_count, pin, pout);

    // notification api
    static struct sigevent notify;
    notify.sigev_notify = SIGEV_SIGNAL;
    notify.sigev_signo = SIGRTMIN;
    notify.sigev_value.sival_ptr = &pin;
    if (mq_notify(pin, &notify) < 0)
        ERR("mq_notify - setting a notification failed");

    parent_work(pout);
    
    mq_close(pin);
    mq_close(pout);

    if (mq_unlink("/bingo_in"))
		ERR("mq unlink");
	if (mq_unlink("/bingo_out"))
		ERR("mq unlink");

    return EXIT_SUCCESS;
}

void sigchld_handler(int sig, siginfo_t *s, void *p)
{
    // wait for children
    pid_t pid;
	for (;;) {
		pid = waitpid(0, NULL, WNOHANG);
		if (pid == 0)
			return;
		if (pid <= 0) {
			if (errno == ECHILD)
				return;
			ERR("waitpid");
		}
		--children_left;
	}
}

void mq_handler(int sig, siginfo_t *info, void *p)
{
    // get info
    mqd_t *pin = (mqd_t *) info->si_value.sival_ptr;

    // set notification again
    static struct sigevent notify;
    notify.sigev_notify = SIGEV_SIGNAL;
    notify.sigev_signo = SIGRTMIN;
    notify.sigev_value.sival_ptr = pin;
    if (mq_notify(*pin, &notify)) {
        ERR("mq_notify");
    }

    // empty the message queue
    for (;;) {
        uint8_t msg;
        unsigned int priority;
        if (mq_receive(*pin, (char *)&msg, 1, &priority) < 1) {
            if (errno == EAGAIN) {
                // if the queue is empty
                break;
            } else {
                ERR("(mq_handler) mq_receive failed");
            }
        }
        if (0 == priority) { // the child lost the game
            printf("MQ: got timeout from %d.\n", msg);
        } else { // the child won the game
            printf("MQ:%d is a bingo number!\n", msg);
        }
    }
    
}

void create_children(int children_count, mqd_t pin, mqd_t pout)
{
    for (int i = 0; i < children_count; ++i) {
        pid_t result = fork();
        if (result == 0) {
            child_work(i, pin, pout);

            mq_close(pin);
            mq_close(pout);

            exit(EXIT_SUCCESS);
        } else if (result == -1) {
            ERR("fork failed");
        }
        ++children_left;
    }
}

void parent_work(mqd_t pout)
{
    srand(getpid());
    while (children_left > 0) {

        uint8_t msg = rand() % MAX_NUM;

        if (-1 == TEMP_FAILURE_RETRY(mq_send(pout, (const char*)&msg, 1, 1))) {
            ERR("mq_send failed");
        }

        sleep(1);
    }
    printf("[Parent] Terminates\n");
}

void child_work(int id, mqd_t pin, mqd_t pout)
{
    srand(getpid());
    int my_life_span = rand() % LIFE_SPAN + 1;
    uint8_t my_bingo = (uint8_t)(rand() % MAX_NUM);
    printf("child with id: %d\n", id);
    
    for (int i = 0; i < my_life_span; ++i) {
        uint8_t msg;
        if (-1 == TEMP_FAILURE_RETRY(mq_receive(pout, (char*)&msg, 1, NULL))) {
            ERR("(child) mq_receive failed");
        }

        printf("[Child: %d] Received %d\n", getpid(), msg);

        if (msg == my_bingo) {
            if (-1 == TEMP_FAILURE_RETRY(mq_send(pin, (const char*)&my_bingo, 1, 1))) {
                ERR("(child) mq_send my_bingo failed");
                return;
            }
        }
    }

    if (TEMP_FAILURE_RETRY(mq_send(pin, (const char*)&id, 1, 0))) {
        ERR("(child) mq_send id failed");
    }
}