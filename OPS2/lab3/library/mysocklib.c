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

int make_socket_UNIX(char* name, int type, struct sockaddr_un *addr)
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

int make_socket_INET(char* name, int type, struct sockaddr_un *addr)
{
    int socketfd;
    if ((socketfd = socket(AF_INET, type, 0)) < 0)
        ERR("mysocklib: socket() error");

    // zero the structure
    memset(addr, 0, sizeof(struct sockaddr_un));

    // TO DO

    return socketfd;
}

int bind_socket(char* name, int socket_family, int type, int backlog)
{
    struct sockaddr_un addr;
    int socketfd;

    // bind will link the socket with the file
    // so we need to remove the file with the same name
    if (unlink(name) < 0 && errno != ENOENT)
        ERR("mysocklib: bind() error");
    
    // chooose the correct family
    if (socket_family == AF_UNIX)
        socketfd = make_socket_UNIX(name, type, &addr);
    else if (socket_family == AF_INET)
        socketfd = make_socket_INET(name, type, &addr);
    else
        ERR("mysocklib: socket_family not supported");

    // bind the socket
    if (bind(socketfd, (struct sockaddr *)&addr, SUN_LEN(&addr)) < 0)
        ERR("mysocklib: bind() error");

    // start listening
    if (listen(socketfd, backlog))
        ERR("mysocklib: listen() error");

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

int connect_socket(char *name, int socket_family, int type)
{
	struct sockaddr_un addr;
	int socketfd;

    if (socket_family == AF_UNIX)
	    socketfd = make_socket_UNIX(name, type, &addr);
    else if (socket_family == AF_INET)
        socketfd = make_socket_INET(name, type, &addr);
    else
        ERR("mysocklib: socket family not supported");

if (connect(socketfd, (struct sockaddr *)&addr, SUN_LEN(&addr)) < 0) {
		if (errno != EINTR)
			ERR("connect");
		else {
			fd_set wfds;
			int status;
			socklen_t size = sizeof(int);
			FD_ZERO(&wfds);
			FD_SET(socketfd, &wfds);
			if (TEMP_FAILURE_RETRY(select(socketfd + 1, NULL, &wfds, NULL, NULL)) < 0)
				ERR("select");
			if (getsockopt(socketfd, SOL_SOCKET, SO_ERROR, &status, &size) < 0)
				ERR("getsockopt");
			if (0 != status)
				ERR("connect");
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