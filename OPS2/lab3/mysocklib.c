#include "mysocklib.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

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

int bind_socket(char* name)
{
    struct sockaddr_un addr;
    int socketfd;
    if (unlink(name) < 0 && errno != ENOENT)

}