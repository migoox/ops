#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
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

#define MAX_NAME_SIZE 16
#define MSG_SIZE 100
#define ERR(source)                                                                                                    \
	(fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), kill(0, SIGKILL), exit(EXIT_FAILURE))

void usage(void)
{
	fprintf(stderr, "USAGE: \n");
	exit(EXIT_FAILURE);
}

void server_communication(mqd_t queue, mqd_t server_queue, char q_name[MAX_NAME_SIZE]);

int main(int argc, char **argv)
{
    if (argc != 2)
        usage();
    char *server_q_name = argv[1];


    // CLIENT
    struct mq_attr attr;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MSG_SIZE;
    mqd_t queue;
    char q_name[MAX_NAME_SIZE];
    sprintf(q_name,"/%d", getpid());
    if ((queue = TEMP_FAILURE_RETRY(mq_open(q_name, O_RDWR | O_CREAT, 0600, &attr))) == (mqd_t)-1) {
        ERR("mq_open failed");
    }
    printf("[Client] Queue (mqd=%d, name=%s) has been created.\n", queue, q_name);

    // SERVER
    mqd_t server_queue;
    if ((server_queue = TEMP_FAILURE_RETRY(mq_open(server_q_name, O_RDWR, 0600, &attr))) == (mqd_t)-1) {
        ERR("mq_open failed");
    }
    printf("[Client] Server queue (mqd=%d, name=%s) has been opened.\n", server_queue, server_q_name);

    server_communication(queue, server_queue, q_name);

    // SERVER
    if (-1 == TEMP_FAILURE_RETRY(mq_close(server_queue))) {
        ERR("mq_close failed");
    }
    printf("[Client] Server queue (mqd=%d, name=%s) has been closed.\n", server_queue, server_q_name);

    // CLIENT
    if (TEMP_FAILURE_RETRY(mq_close(queue)) == (mqd_t)-1) {
        ERR("mq_close failed");
    }
    printf("[Client] Queue (mqd=%d, name=%s) has been closed.\n", queue, q_name);

    if (mq_unlink(q_name)) {
        ERR("mq_unlink failed");
    }
    printf("[Client] Queue (name=%s) has been unlinked.\n", q_name);

    return EXIT_SUCCESS;
}


void server_communication(mqd_t queue, mqd_t server_queue, char q_name[MAX_NAME_SIZE])
{
    static char stdin_buff[MSG_SIZE];
    static char buff[MSG_SIZE];

    while (fgets(stdin_buff, MSG_SIZE, stdin) != NULL) {

        char* endptr;
        errno = 0;
        uint32_t msg = strtol(stdin_buff, &endptr, 10);
        if (errno != 0) {
            perror("strtol");
            exit(EXIT_FAILURE);
        }

        if (endptr == stdin_buff) { // it is not a number
            continue;
        }

		printf("[Client] Input: %d\n", msg);

        *((uint32_t *)buff) = msg;
        memcpy(buff + sizeof(uint32_t), q_name, MAX_NAME_SIZE);

        if (-1 == TEMP_FAILURE_RETRY(mq_send(server_queue, buff, MSG_SIZE, 1))) {
            ERR("send failed");
        }

        if (-1 == TEMP_FAILURE_RETRY(mq_receive(queue, buff, MSG_SIZE, NULL))) {
            ERR("receive failed");
        } 
        printf("[Client] Server response: %s\n", buff);
    }
}