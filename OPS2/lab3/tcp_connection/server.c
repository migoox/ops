#define _GNU_SOURCE
#include "../library/mysocklib.h"
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#define BACKLOG 3

// indicates whether the server is running
volatile sig_atomic_t do_work = 1;

void sigint_handler(int sig);
void usage(char *name);
void do_server(int serverfd_local, int serverfd_tcp);
void communicate(int clientfd);
void calculate(int32_t data[5]);
int main(int argc, char **argv)
{
    if (argc != 3) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    // ignore the SIGPIPE
    if (sethandler(SIG_IGN, SIGPIPE))
        ERR("setting SIGPIPE");
    // set sigint
    if (sethandler(sigint_handler, SIGINT))
        ERR("setting SIGINT");

    int serverfd_local = LOCAL_bind_socket(argv[1], SOCK_STREAM, BACKLOG);

    // get current flags for the serverfd and add O_NONBLOCK and set new flags
    // F_GETFL, F_SETFL - status flags and file access modes (man 3p fcntl)
    // fcntl is a variadic function (unspecified number of args)
    int new_flags = fcntl(serverfd_local, F_GETFL) | O_NONBLOCK;
    fcntl(serverfd_local, F_SETFL, new_flags);

    int serverfd_tcp = TCP_IPv4_bind_socket(atoi(argv[2]), BACKLOG);
    // set nonblock
    new_flags = fcntl(serverfd_tcp, F_GETFL) | O_NONBLOCK;
    fcntl(serverfd_tcp, F_SETFL, new_flags);

    do_server(serverfd_local, serverfd_tcp);

	if (TEMP_FAILURE_RETRY(close(serverfd_local)) < 0)
		ERR("close");
        
	if (unlink(argv[1]) < 0)
		ERR("unlink");

	if (TEMP_FAILURE_RETRY(close(serverfd_tcp)) < 0)
		ERR("close");

    fprintf(stderr, "Server has terminated.\n");

    return EXIT_SUCCESS;
}

void do_server(int serverfd_local, int serverfd_tcp)
{
    // man select: Upon return, each of the file descriptor sets is modified in place to
    // indicate which file descriptors are currently "ready". Thus, if using  select()
    // within a loop, the sets must be reinitialized before each call.
    fd_set base_rfds;

    // initalize teh set
    FD_ZERO(&base_rfds);

    // add socket of the server to the mask
    FD_SET(serverfd_local, &base_rfds);
    FD_SET(serverfd_tcp, &base_rfds);
    int fdmax = (serverfd_tcp > serverfd_local ? serverfd_tcp : serverfd_local);

    // ignore SIGINT if pselect is not running
    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
	sigprocmask(SIG_BLOCK, &mask, &oldmask);

    while (do_work) {
        fd_set rfds = base_rfds;
        // wait for serverfd_local being ready to read 
        if (pselect(fdmax + 1, &rfds, NULL, NULL, NULL, &oldmask) > 0) {
            int clientfd;

            if (FD_ISSET(serverfd_local, &rfds)) 
                clientfd = add_new_client(serverfd_local);
            else
                clientfd = add_new_client(serverfd_tcp);

            if (clientfd >= 0)
                communicate(clientfd);
        } else {
            if (EINTR == errno)
                continue;

            ERR("pselect");
        }
    }
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

void communicate(int clientfd)
{
    const ssize_t data_size = sizeof(int32_t[5]);

    ssize_t size;
	int32_t data[5];
                
    // read from the client
    if ((size = bulk_read(clientfd, (char *)data, data_size)) < 0)
        ERR("read error");    
    
    // calculate and give the answer
    if (size == data_size) {
        calculate(data);
        if (bulk_write(clientfd, (char *)data, data_size) < 0)
            ERR("write error");
    }

    // close the client
    if (TEMP_FAILURE_RETRY(close(clientfd)) < 0)
        ERR("close");
}

void calculate(int32_t data[5])
{
    int32_t op1, op2, result, status = 1;
    
    // network byte-order to host-order, l means uint32_t
    op1 = ntohl(data[0]);
    op2 = ntohl(data[1]);

    // calculate
    switch ((char)ntohl(data[3])) {
        case '+':
            result = op1 + op2;
            break;
        case '-':
		    result = op1 - op2;
		    break;
	    case '*':
		    result = op1 * op2;
		    break;
	    case '/':
		    if (0 == op2)
			    status = 0;
		    else
			    result = op1 / op2;
		    break;
	    default:
		    status = 0;
	}   

    // host byte-order to network byte-order
    data[4] = htonl(status);
    data[2] = htonl(result);
}

void sigint_handler(int sig)
{
    // if SIGINT is received we shutdown the server
    do_work = 0;
}

void usage(char *name)
{
	fprintf(stderr, "USAGE: %s socket port\n", name);
}