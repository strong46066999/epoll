#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoll.h"
#include "socket.h"

#define SERVER_PORT 3000
#define MAX_BACKLOG 10
#define SOCKET_TIMEOUT 100

extern fde fd_table[];
int serverfd;

void serverWrite(int fd, void* data) 
{
    printf("serverWrite on FD %d\n");
    send(fd, "123", 4, 0);
}

void serverRead(int fd, void* data)
{
    char buff[1024];
    int len = 0;

    len = recv(fd, buff, sizeof(buff), 0);
    printf("serverRead on FD %d len = %d\n", fd, len);
    epollSetSelect(fd, SELECT_READ, serverRead, NULL, SOCKET_TIMEOUT);
    epollSetSelect(fd, SELECT_WRITE, serverWrite, NULL, SOCKET_TIMEOUT);
}

void serverAccept(int fd, void* data) 
{
    int retval, newfd;
    struct sockaddr_in client_addr;

    if((newfd = accept(fd, (struct sockaddr*)&client_addr, &retval)) > 0) {
        fde *F = &fd_table[newfd];
        F->flags.open = 1;
        printf("serverAccept on FD %d\n", newfd);
        epollSetSelect(newfd, SELECT_READ, serverRead, NULL, SOCKET_TIMEOUT);
        epollSetSelect(newfd, SELECT_WRITE, serverWrite, NULL, SOCKET_TIMEOUT);
        setNonBlocking(newfd);
    }

    epollSetSelect(serverfd, SELECT_READ, serverAccept, NULL, SOCKET_TIMEOUT);
}

int main(int argc, char** argv) 
{
    struct sockaddr_in localip;
    int retval;
    
    localip.sin_family = AF_INET;
    localip.sin_addr.s_addr = INADDR_ANY;
    localip.sin_port = htons(SERVER_PORT);

    if((serverfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("socket:failed :%s\n", strerror(errno));
        exit(-1);
    }

    fde *F = &fd_table[serverfd];
    F->flags.open = 1;

    setNonBlocking(serverfd);
    setAddrReuse(serverfd);

    if(bind(serverfd, (struct sockaddr*)&localip, sizeof(localip)) < 0) {
        printf("bind:failed on FD %d :%s\n", serverfd, strerror(errno));
        exit(-1);
    }

    if(listen(serverfd, MAX_BACKLOG) < 0) {
        printf("listen:failed on FD %d :%s\n", serverfd, strerror(errno));
        exit(-1);
    }
    
    epollInit();

    epollSetSelect(serverfd, SELECT_READ, serverAccept, NULL, SOCKET_TIMEOUT);

    while(1) {
        epollDoSelect(SOCKET_TIMEOUT);
    }

    return 0;
}
