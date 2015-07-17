#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <sys/socket.h>

#include "epoll.h"

extern fde fd_table[];

int setNonBlocking(int fd)
{
    int flags;
    int dummy = 0;

    if((flags = fcntl(fd, F_GETFL, dummy)) < 0) {
        printf("FD %d: fcntl F_GETFL: %s\n", fd, strerror(errno));
        return -1;
    }

    if(fcntl(fd, F_SETFL, flags|O_NONBLOCK) < 0) {
        printf("FD %d: fcntl F_SETFL: %s\n", fd, strerror(errno));
        return -1;        
    }

    fd_table[fd].flags.nonblocking = 1;
    return 0;    
}

int setAddrReuse(int fd)
{
    int flags = 1;
    
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags)) < 0) {
        printf("FD %d: setsockopt SO_REUSEADDR: %s\n", fd, strerror(errno));
        return -1;        
    }

    fd_table[fd].flags.addr_reuse = 1;
    return 0;
}
