#ifndef MYSOCKLIB_H_
#define MYSOCKLIB_H_
#include <sys/socket.h>
#include <sys/un.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

int sethandler(void (*f)(int), int sig_no);

int LOCAL_make_socket(char* name, int type, struct sockaddr_un *addr);

// makes socket, binds it and starts listening for the connections
int LOCAL_bind_socket(char* name, int type, int backlog);

// returns client fd on success, or -1 if there is no connection (nonblock)
// if serverfd is set to blocking, this function will block and -1 won't be 
// ever returned
int LOCAL_add_new_client(int serverfd);

// makes socket, and connects it 
int LOCAL_connect_socket(char *name, int type);


int TCP_IPv4_make_socket(char* name, int type, struct sockaddr_un *addr);

int TCP_IPv4_bind_socket(char* name, int type, int backlog);

int TCP_IPv4_add_new_client(int serverfd);

int TCP_IPv4_connect_socket(char *name, int type);

// signal-resistant reading, if the reading is interrupted
// by the signal, it will retry reading.
// reads maximaly count bytes and stores them in the buf
ssize_t bulk_read(int fd, char *buf, size_t count);

// signal-resistant writing, if the writing is interrupted
// by the signal, it will retry writing.
// writes count bytes and from the buf
ssize_t bulk_write(int fd, char *buf, size_t count);

#endif