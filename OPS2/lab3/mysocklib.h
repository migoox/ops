#ifndef MYSOCKLIB_H_
#define MYSOCKLIB_H_
#include <sys/socket.h>
#include <sys/un.h>

#define ERR(source) (perror(source), fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), exit(EXIT_FAILURE))

int make_socket_UNIX(char* name, int type, struct sockaddr_un *addr);

#endif