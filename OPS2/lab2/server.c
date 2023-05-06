#define _GNU_SOURCE
#include <fcntl.h>
#include <errno.h>
#include <mqueue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>

#define ERR(source)                                                                                                    \
	(fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

#define QUEUES_COUNT 4
#define MSG_SIZE 100
#define MAX_NAME_SIZE 16

void usage(void)
{
	fprintf(stderr, "USAGE: \n");
	fprintf(stderr, "0<d_i\n");
	exit(EXIT_FAILURE);
}

typedef struct queue_info {
    mqd_t descriptor;
    int divisor;
    char name[MAX_NAME_SIZE];
} queue_info_t;

void initialize_server(queue_info_t queues[QUEUES_COUNT]);
void close_server(queue_info_t queues[QUEUES_COUNT]);
void thread_notification_handler(union sigval sv);

int main(int argc, char **argv)
{
    srand(time(NULL));
    if (argc != QUEUES_COUNT + 1)
        usage();
    
    queue_info_t queues[QUEUES_COUNT];
    for (int i = 0; i < QUEUES_COUNT; ++i) {
        if ((queues[i].divisor = atoi(argv[i + 1])) < 1)
            usage();
    }
    
    initialize_server(queues);

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }
    // Wait for SIGINT signal
    int sig;
    sigwait(&set, &sig);
    printf("[Server] Received SIGINT signal.\n");


    close_server(queues);

    return EXIT_SUCCESS;
}

void initialize_server(queue_info_t queues[QUEUES_COUNT])
{
struct mq_attr attr;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MSG_SIZE;

    for(int i = 0; i < QUEUES_COUNT; ++i) {  
        // prepare current name
        sprintf(queues[i].name, "/%d_%d", getpid(), queues[i].divisor);

        // create
        if ((queues[i].descriptor = TEMP_FAILURE_RETRY(mq_open(queues[i].name, O_RDWR | O_NONBLOCK | O_CREAT, 0600, &attr))) == (mqd_t)-1) {
            ERR("mq_open failed");
        }

        printf("[Server] Queue (mqd=%d, divisor=%d, name=%s) has been created.\n", queues[i].descriptor, queues[i].divisor, queues[i].name);
    }

    // set notifications
    for(int i = 0; i < QUEUES_COUNT; ++i) {  
        struct sigevent not;
        not.sigev_notify = SIGEV_THREAD;
        not.sigev_notify_function = thread_notification_handler; 
        not.sigev_notify_attributes = NULL;
        not.sigev_value.sival_ptr = (void*)&queues[i];
        if (mq_notify(queues[i].descriptor, &not)) {
            ERR("mq_notify failed");
        }
    }
}

void close_server(queue_info_t queues[QUEUES_COUNT])
{

    for(int i = 0; i < QUEUES_COUNT; ++i) {  

        if (-1 == TEMP_FAILURE_RETRY(mq_close(queues[i].descriptor))) {
            ERR("mq_close failed");
        }
        printf("[Server] Queue (mqd=%d, divisor=%d, name=%s) has been closed.\n", queues[i].descriptor, queues[i].divisor, queues[i].name);
    }

    for(int i = 0; i < QUEUES_COUNT; ++i) {  
        // create
        if (mq_unlink(queues[i].name)) {
            ERR("mq_unlink failed");
        }
        printf("[Server] Queue (divisor=%d, name=%s) has been unlinked.\n", queues[i].divisor, queues[i].name);
    }
}

void thread_notification_handler(union sigval sv)
{
    queue_info_t *info = (queue_info_t *) sv.sival_ptr;

    // set notification again
    struct sigevent not;
    not.sigev_notify = SIGEV_THREAD;
    not.sigev_notify_function = thread_notification_handler; 
    not.sigev_notify_attributes = NULL;
    not.sigev_value.sival_ptr = info;
    if (mq_notify(info->descriptor, &not)) {
        ERR("mq_notify failed");
    }

     // empty the message queue
    for (;;) {
        char buffer[MSG_SIZE];
        if (mq_receive(info->descriptor, buffer, MSG_SIZE, NULL) < 1) {
            if (errno == EAGAIN) {
                // if the queue is empty
                break;
            } else {
                ERR("mq_receive failed");
            }
        }
        uint32_t number = *((uint32_t *)buffer);
        char* client_q_name = buffer + sizeof(uint32_t);
        printf("[Server] Received: %d, from client %s on queue with divisor %d\n", number, client_q_name, info->divisor);


        mqd_t client_des;
        struct mq_attr attr;
        attr.mq_maxmsg = 10;
        attr.mq_msgsize = MSG_SIZE;
        if ((client_des= TEMP_FAILURE_RETRY(mq_open(client_q_name, O_RDWR | O_NONBLOCK, 0600, &attr))) == (mqd_t)-1) {
            ERR("mq_open failed");
        }

        if (number % info->divisor == 0) {
            strcpy(buffer, "DIVISIBLE");
        } else {
            strcpy(buffer, "NON-DIVISIBLE");
        }

        if (-1 == TEMP_FAILURE_RETRY(mq_send(client_des, buffer, MSG_SIZE, 1))) {
            ERR("mq_send failed");
        }

        if (-1 == TEMP_FAILURE_RETRY(mq_close(client_des))) {
            ERR("mq_close failed");
        }
    }
}