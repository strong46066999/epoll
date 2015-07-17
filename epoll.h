#ifndef SDCM_EPOLL_H
#define SDCM_EPOLL_H

#define MAXFD   8192

#define SELECT_READ (0x1)
#define SELECT_WRITE (0x2)

typedef void PF(int, void*);

static int kdpfd = -1;
static int max_poll_time = 1000;
static struct epoll_event *pevents;

typedef struct _fde {
    struct {
        unsigned open:1;
        unsigned read_pending:1;
        unsigned nonblocking:1;
        unsigned addr_reuse:1;
    } flags;

    unsigned epoll_state;
    
    PF* read_handler;
    void* read_data;

    PF* write_handler;
    void* write_data;
}fde;

void epollInit(void);
void epollSetSelect(int fd, unsigned int type, PF* handler, void* client_data, time_t timeout);
void epollResetSelect(int fd);
void epollDoSelect(int msec);

#endif
