#define _GNU_SOURCE
#include "mysocklib.h"
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>

int LOCAL_make_socket(char* name, int type, struct sockaddr_un *addr)
{
    int socketfd;
    if ((socketfd = socket(AF_UNIX, type, 0)) < 0)
        ERR("mysocklib: socket() error");

    // zero the structure
    memset(addr, 0, sizeof(struct sockaddr_un));

    // set socket's family to UNIX
    addr->sun_family = AF_UNIX;

    // set socket's name
    strncpy(addr->sun_path, name, sizeof(addr->sun_path) - 1);

    return socketfd;
}

int LOCAL_bind_socket(char* name, int type, int backlog)
{
    struct sockaddr_un addr;
    int socketfd;

    // bind will link the socket with the file
    // so we need to remove the file with the same name
    if (unlink(name) < 0 && errno != ENOENT)
        ERR("mysocklib: bind() error");
    
    socketfd = LOCAL_make_socket(name, type, &addr);

    // bind the socket
    if (bind(socketfd, (struct sockaddr *)&addr, SUN_LEN(&addr)) < 0)
        ERR("mysocklib: bind() error");

    // start listening
    if (listen(socketfd, backlog))
        ERR("mysocklib: listen() error");

    return socketfd;
}

int LOCAL_connect_socket(char *name, int type)
{
	struct sockaddr_un addr;
	int socketfd;

	socketfd = LOCAL_make_socket(name, type, &addr);

    // add this client socket to the listen queue, may run asynchronously
    // sizeof would return bigger value than SUN_LEN (includes bytes of the gaps), 
    // but sizeof isn't considered as a mistake
    if (connect(socketfd, (struct sockaddr *)&addr, SUN_LEN(&addr)) < 0) {
		if (EINTR != errno) {
			ERR("mysocklib: connect() error");
		} else {
			fd_set write_fd_set;
			int status;
			socklen_t size = sizeof(int);

            // initialize the set
			FD_ZERO(&write_fd_set);

            // add socketfd to the set of descriptors that we will be wating for
			FD_SET(socketfd, &write_fd_set);

            // we need to wait for the server to accept this connection (connect() may run async, for example if it 
            // is interrupted by the signal), so we have to monitor if socketfd inlcuded int write_fd_set is ready
            // MANPAGE: A file descriptor is considered ready if it is
            // possible to perform a corresponding I/O operation (e.g., read(2), or a sufficiently small write(2)) without blocking.
            // in the case of an error, pselect also returns.
            // we use select in both blocking and non-blocking states
			if (TEMP_FAILURE_RETRY(select(socketfd + 1, NULL, &write_fd_set, NULL, NULL)) < 0)
				ERR("mysocklib: select() error");

            // get status of the socket to indicate whether an error occured
			if (getsockopt(socketfd, SOL_SOCKET, SO_ERROR, &status, &size) < 0)
				ERR("mysocklib: getsockopt() error");

            // check if error occured
			if (0 != status)
				ERR("mysocklib: connect() error");
		}
	}
	return socketfd;
}

struct sockaddr_in IPv4_make_address(char *address, char *port)
{
	int ret;
	struct sockaddr_in addr;
	struct addrinfo *result;
	struct addrinfo hints = {};
	hints.ai_family = AF_INET;
	if ((ret = getaddrinfo(address, port, &hints, &result))) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
		exit(EXIT_FAILURE);
	}
	addr = *(struct sockaddr_in *)(result->ai_addr);
    //  printf("%s\n", inet_ntoa (addr.sin_addr));
	freeaddrinfo(result);
	return addr;
}

int TCP_IPv4_make_socket(void)
{
    int socketfd;
    if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        ERR("mysocklib: socket() error");

    return socketfd;
}

int TCP_IPv4_bind_socket(uint16_t port, int backlog)
{
    struct sockaddr_in addr;
	int socketfd, status = 1;

    socketfd = TCP_IPv4_make_socket();

    // zero the structure
    memset(&addr, 0, sizeof(struct sockaddr_in));

    // set the structure
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // For AF_INET  sockets this means that a socket may bind, except when 
    // there is an active listening socket bound to the address.
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &status, sizeof(status)))
        ERR("mysocklib: setsockopt() error");
    
    if (bind(socketfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        ERR("mysocklib: bind() error");

    if (listen(socketfd, backlog))
        ERR("mysocklib: listen() error");

    return socketfd;
}

int TCP_IPv4_connect_socket(char *name, char *port)
{
    struct sockaddr_in addr;
	int socketfd;

    socketfd = TCP_IPv4_make_socket();
    addr = IPv4_make_address(name, port);

    if (connect(socketfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
        if (EINTR != errno) {
            ERR("mysocklib: connect() error");
        } else {
			fd_set write_fd_set;
			int status;
			socklen_t size = sizeof(int);

            // initialize the set
			FD_ZERO(&write_fd_set);

            // add socketfd to the set of descriptors that we will be wating for
			FD_SET(socketfd, &write_fd_set);

            // we need to wait for the server to accept this connection (connect() may run async, for example if it 
            // is interrupted by the signal), so we have to monitor if socketfd inlcuded int write_fd_set is ready
            // MANPAGE: A file descriptor is considered ready if it is
            // possible to perform a corresponding I/O operation (e.g., read(2), or a sufficiently small write(2)) without blocking.
            // in the case of an error, pselect also returns.
            // we use select in both blocking and non-blocking states
			if (TEMP_FAILURE_RETRY(select(socketfd + 1, NULL, &write_fd_set, NULL, NULL)) < 0)
				ERR("mysocklib: select() error");

            // get status of the socket to indicate whether an error occured
			if (getsockopt(socketfd, SOL_SOCKET, SO_ERROR, &status, &size) < 0)
				ERR("mysocklib: getsockopt() error");

            // check if error occured
			if (0 != status)
				ERR("mysocklib: connect() error");
        }
    }

    return socketfd;
}

int add_new_client(int serverfd)
{
    int clientfd;

    // extract firt element in the server's queue of pending connections
    // and create a client socket
    if ((clientfd = TEMP_FAILURE_RETRY(accept(serverfd, NULL, NULL))) < 0) {

        // if serverfd is set to nonblock and there is no connection
        if (EAGAIN == errno || EWOULDBLOCK == errno)
            return -1;
        
        // if error occured    
        ERR("mysocklib: accept() error");
    }

    return clientfd;
}

// works with both - blocking and non-blocking states 
ssize_t bulk_read(int fd, char *buf, size_t count)
{
    int read_result;
    fd_set base_rfds; 
    FD_ZERO(&base_rfds);
    FD_SET(fd, &base_rfds);

    size_t len = 0;

    while (count > 0) {
        fd_set rfds = base_rfds;

        if (TEMP_FAILURE_RETRY(select(fd + 1, &rfds, NULL, NULL, NULL)) < 0)
				ERR("mysocklib: select() error");

        read_result = TEMP_FAILURE_RETRY(read(fd, buf, count));
        if (read_result < 0)
            ERR("mysocklib: read() error");
    
        // move pointer
        buf += read_result;
        len += read_result;
        count -= read_result;
    } 
    
    return len;
}

// works with both - blocking and non-blocking states 
ssize_t bulk_write(int fd, char *buf, size_t count)
{
    int write_result;
    fd_set base_wfds; 
    FD_ZERO(&base_wfds);
    FD_SET(fd, &base_wfds);

    size_t len = 0;
    
    while (count > 0) {
        fd_set wfds = base_wfds;

        if (TEMP_FAILURE_RETRY(select(fd + 1, NULL, &wfds, NULL, NULL)) < 0)
				ERR("mysocklib: select() error");

        write_result = TEMP_FAILURE_RETRY(write(fd, buf, count));
        if (write_result < 0)
            ERR("mysocklib: write() error");
    
        // move pointer
        buf += write_result;
        len += write_result;
        count -= write_result;
    } 
    
    return len;
}

int sethandler(void (*f)(int), int sig_no)
{
    struct sigaction act;

	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	
    if (-1 == sigaction(sig_no, &act, NULL))
		return -1;

	return 0;
}
