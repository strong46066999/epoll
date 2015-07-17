#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <assert.h>

#include <sys/epoll.h>

#include "epoll.h"

fde fd_table[MAXFD];

void epollInit(void) 
{
    pevents = (struct epoll_event *) malloc(MAXFD * sizeof(struct epoll_event));
    if(!pevents) {
        printf("epoll_init: malloc() faled: %s\n", strerror(errno));
    }
    
    kdpfd = epoll_create(MAXFD);
    if(kdpfd < 0) {
        printf("epoll_init: epoll_create(): %s\n", strerror(errno));
    }
}

static const char* epolltype_atoi(int x) 
{
    switch (x) {
        case EPOLL_CTL_ADD:
            return "EPOLL_CTL_ADD";
        case EPOLL_CTL_DEL:
            return "EPOLL_CTL_DEL";
        case EPOLL_CTL_MOD:
            return "EPOLL_CTL_MOD";
        default:
            return "UNKNOWN_EPOLLCTL_OP";
    }
}

void epollSetSelect(int fd, unsigned int type, PF* handler, void* client_data, time_t timeout)
{
    fde *F = &fd_table[fd];
    int epoll_ctl_type = 0;

    assert(fd >= 0);
    printf("FD %d, type=%d, handler=%p, client_data=%p," \
            "timeout=%u\n", fd, type, handler, client_data, timeout);

    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.data.fd = fd;
    
    if(!F->flags.open) {
        epoll_ctl(kdpfd, EPOLL_CTL_DEL, fd, &ev);
        return;
    }

    // If read is an interest
    if(type & SELECT_READ) {
        if(handler) {
        // Hack to keep the events flowing if there is data immediately ready
            if(F->flags.read_pending) 
                ev.events |= EPOLLOUT;

            ev.events |= EPOLLIN;
        }

        F->read_handler = handler;
        F->read_data = client_data;
    // Otherwise, use previously stored value        
    } else if(F->epoll_state & EPOLLIN) {
        ev.events |= EPOLLIN;
    }

    // If write is an interest
    if(type & SELECT_WRITE) {
        if(handler) 
            ev.events |= EPOLLOUT;

        F->write_handler = handler;
        F->write_data = client_data;
    // Otherwise, use previously stored value
    } else if(F->epoll_state & EPOLLOUT) {
        ev.events |= EPOLLOUT;
    }

    if(ev.events)
        ev.events |= EPOLLHUP | EPOLLERR;

    if(ev.events != F->epoll_state) {
        if(F->epoll_state) // Already monitoring something.
            epoll_ctl_type = ev.events ? EPOLL_CTL_MOD:EPOLL_CTL_DEL;
        else
            epoll_ctl_type = EPOLL_CTL_ADD;

        F->epoll_state = ev.events;

        if(epoll_ctl(kdpfd, epoll_ctl_type, fd, &ev) < 0)
            printf("epoll_ctl(,,,%d,):failed on FD:%s\n", fd, strerror(errno));
    }
}

void epollResetSelect(int fd)
{
    fde *F = &fd_table[fd];
    F->epoll_state = 0;
    epollSetSelect(fd, 0, NULL, NULL, 0);    
}

void epollDoSelect(int msec)
{
    int num, i, fd;
    fde *F;
    PF* hdl;
    
    struct epoll_event* cevents;
    
    if(msec > max_poll_time)
        msec = max_poll_time;
    
    for(;;) {
        num = epoll_wait(kdpfd, pevents, MAXFD, msec);
        
        if(num == 0)
            return;

        for(i = 0, cevents = pevents; i < num; ++i, ++cevents) {
            fd = cevents->data.fd;
            F = &fd_table[fd];
            printf("got FD %d events=%d monitoring=%d " \
                    "F->read_handler=%p F->write_handler=%p\n", fd, \
                    cevents->events, F->epoll_state, \
                    F->read_handler, F->write_handler);

            if(cevents->events & (EPOLLIN|EPOLLHUP|EPOLLERR) || F->flags.read_pending) {
                if((hdl = F->read_handler) != NULL) {
                    printf("Calling read handler on FD %d\n", fd);
                    F->flags.read_pending = 0;
                    F->read_handler = NULL;
                    hdl(fd, F->read_data);
                } else {
                    printf("no read handler for FD %d\n", fd);
                    epollSetSelect(fd, SELECT_READ, NULL, NULL, 0);
                }
            }

            if(cevents->events & (EPOLLOUT|EPOLLHUP|EPOLLERR)) {
                if((hdl = F->write_handler) != NULL) {
                    printf("Calling write handler on FD %d\n", fd);
                    F->write_handler = NULL;
                    hdl(fd, F->read_data);
                } else {
                    printf("no write handler for FD %d\n", fd);
                    epollSetSelect(fd, SELECT_WRITE, NULL, NULL, 0);
                }
            }
        }
    }
}
