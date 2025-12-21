#ifndef IO_H
#define IO_H

#include <stddef.h>
#include <sys/socket.h>
#include <sys/types.h>


void io_init(void);

// Non-blocking wrappers
ssize_t gthread_read(int fd, void *buf, size_t count);
ssize_t gthread_write(int fd, const void *buf, size_t count);
int gthread_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

// Register wait
// Wait for events (POLLIN/POLLOUT) on fd. Blocks current thread.
void gthread_wait_io(int fd, int events);

#endif
