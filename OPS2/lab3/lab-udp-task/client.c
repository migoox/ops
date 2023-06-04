#define _GNU_SOURCE
#include "../../mysocklib/mysocklib.h"
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFF_SIZE 128

// Warning: range should fit in int32_t
#define RANGE_LEFT 1000000
#define RANGE_RIGHT 10000000

volatile sig_atomic_t last_signal = 0;

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s domain port file \n", name);
}

void sigalrm_handler(int sig)
{
	last_signal = sig;
}

void send_and_confirm(int clientfd, struct sockaddr_in server_addr)
{
    // prepare data
    uint32_t rand_num = RANGE_LEFT + rand() % (RANGE_RIGHT - RANGE_LEFT);
    fprintf(stderr, "[Client] Generated number: %d\n", rand_num);

    // pack the data
    char buff[BUFF_SIZE];
    ((uint32_t *)buff)[0] = htonl(rand_num);

    // UDP send is atomic, so we don't n`eed bulk_write
    // EINTR  A signal interrupted sendto() BEFORE any data was transmitted. (man 3p sendto)
    // EPIPE is critical error here
    if (TEMP_FAILURE_RETRY(sendto(clientfd, buff, BUFF_SIZE, 0,
               &server_addr, sizeof(struct sockaddr_in))) < 0) {
        ERR("sendto");
    }

    // set SIGALRM
	struct itimerval ts;
	memset(&ts, 0, sizeof(struct itimerval));
	ts.it_value.tv_usec = 500000;
    ts.it_value.tv_sec = 1;
	setitimer(ITIMER_REAL, &ts, NULL);
	last_signal = 0;

	// recv the confirmation datagram from the server
	while (recv(clientfd, buff, BUFF_SIZE, 0) < 0) {
		// if interrupted - try again
		if (EINTR != errno)
			ERR("recv:");

		// if time expired, exit the function -> sending failed
		if (SIGALRM == last_signal){
            fprintf(stderr, "[Client] Time expired: no answer was received\n");
            return;
        }
	}

    fprintf(stderr, "[Client] Received confirmation frame\n");
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    int clientfd = UDP_IPv4_make_socket();
    struct sockaddr_in server_addr = IPv4_make_address(argv[1], argv[2]);
    srand(time(NULL));

    if (sethandler(sigalrm_handler, SIGALRM))
		ERR("Seting SIGALRM:");

    fprintf(stderr, "[Client] Started\n");

    send_and_confirm(clientfd, server_addr);

    if (close(clientfd) < 0) {
        ERR("close");
    }

    fprintf(stderr, "[Client] Closed\n");

    return EXIT_SUCCESS;
}