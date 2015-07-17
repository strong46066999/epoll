.PHONY: all clean

INCLUDES=
LIBS= -lpthread -lc

CC_FLAGS=-g -O0 -fPIC

EPOLL_SOURCES = epoll.c \
	socket.c \
	server.c

EPOLL_OBJECTS = epoll.o \
	socket.o \
	server.o

TARGET=epoll

all:${EPOLL_SOURCES} ${EPOLL_OBJECTS}
	${CC} ${CC_FLAGS} -o ${TARGET} ${EPOLL_OBJECTS} ${LIBS}
	cp -f ${TARGET} bin

clean:
	rm -rf ${EPOLL_OBJECTS} ${TARGET} *~

${EPOLL_OBJECTS}: %.o: %.c
	$(CC) -c $(CC_FLAGS) $< -o $@ ${INCLUDES}
