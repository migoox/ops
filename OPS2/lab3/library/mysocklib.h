#ifndef MYSOCKLIB_H_
#define MYSOCKLIB_H_
#include <sys/socket.h>
#include <sys/un.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

int sethandler(void (*f)(int), int sig_no);

int make_socket_UNIX(char* name, int type, struct sockaddr_un *addr);

int make_socket_INET(char* name, int type, struct sockaddr_un *addr);

// makes socket, binds it and starts listening for the connections
int bind_socket(char* name, int socket_family, int type, int backlog);

// returns client fd on success, or -1 if there is no connection (nonblock)
// if serverfd is set to blocking, this function will block and -1 won't be 
// ever returned
int add_new_client(int serverfd);

int connect_socket(char *name, int socket_family, int type);

// signal-resistant reading, if the reading is interrupted
// by the signal, it will retry reading.
// reads maximaly count bytes and stores them in the buf
ssize_t bulk_read(int fd, char *buf, size_t count);

// signal-resistant writing, if the writing is interrupted
// by the signal, it will retry writing.
// writes count bytes and from the buf
ssize_t bulk_write(int fd, char *buf, size_t count);

#endif