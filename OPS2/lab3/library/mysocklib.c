#define _GNU_SOURCE
#include "mysocklib.h"
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

int sethandler(void (*f)(int), int sig_no)
{
    struct sigaction act;

	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	
    if (-1 == sigaction(sig_no, &act, NULL))
		return -1;

	return 0;
}

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

int LOCAL_add_new_client(int serverfd)
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

int LOCAL_connect_socket(char *name, int type)
{
	struct sockaddr_un addr;
	int socketfd;

	socketfd = LOCAL_make_socket(name, type, &addr);

    // add this client socket to the listen queue, may run asynchronously
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

            // we need to wait for the server to accept this connection, so we have to monitor if socketfd inlcuded int write_fd_set is ready
            // MANPAGE: A file descriptor is considered ready if it is
            // possible to perform a corresponding I/O operation (e.g., read(2), or a sufficiently small write(2)) without blocking.
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

ssize_t bulk_read(int fd, char *buf, size_t count)
{
    int read_result;
    size_t len = 0;

    do {
        read_result = TEMP_FAILURE_RETRY(read(fd, buf, count));

        if (read_result < 0)
            ERR("mysocklib: read() error");
        
        // if there was nothing to read
        if (0 == read_result)
            return len;

        // move pointer
        buf += read_result;
        len += read_result;

        count -= read_result;
    } while(count > 0);
    
    return len;
}

ssize_t bulk_write(int fd, char *buf, size_t count)
{
    int write_result;
    size_t len = 0;
    
    do {
        write_result = TEMP_FAILURE_RETRY(write(fd, buf, count));

        if (write_result < 0)
            ERR("mysocklib: read() error");
    
        // move pointer
        buf += write_result;
        len += write_result;

        count -= write_result;
    } while(count > 0);
    
    return len;
}


int TCP_IPv4_make_socket(char* name, int type, struct sockaddr_un *addr)
{
    return -1;
}

int TCP_IPv4_bind_socket(char* name, int type, int backlog)
{
    return -1;
}

int TCP_IPv4_add_new_client(int serverfd)
{
    return -1;
}

int TCP_IPv4_connect_socket(char *name, int type)
{
    return -1;
}