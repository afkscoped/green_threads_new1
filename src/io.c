#include "io.h"
#include "gthread.h"
#include "scheduler.h" // For wait_io integration?
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

// Actually we need to call scheduler functionality.
// We can expose scheduler_register_io(fd, events, thread) via header or
// scheduler.h

extern void scheduler_register_io_wait(int fd, int events);

void io_init(void) {
  // Nothing?
}

static void set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

ssize_t gthread_read(int fd, void *buf, size_t count) {
  set_nonblocking(fd);
  while (1) {
    ssize_t n = read(fd, buf, count);
    if (n < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        scheduler_register_io_wait(fd, POLLIN); // Block and wait
        gthread_yield();                        // Switch
        continue;                               // Retry
      }
    }
    return n;
  }
}

ssize_t gthread_write(int fd, const void *buf, size_t count) {
  set_nonblocking(fd);
  while (1) {
    ssize_t n = write(fd, buf, count);
    if (n < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        scheduler_register_io_wait(fd, POLLOUT);
        gthread_yield();
        continue;
      }
    }
    return n;
  }
}

int gthread_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
  set_nonblocking(sockfd);
  while (1) {
    int fd = accept(sockfd, addr, addrlen);
    if (fd < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        scheduler_register_io_wait(sockfd, POLLIN);
        gthread_yield();
        continue;
      }
    } else {
      set_nonblocking(fd); // New socket should be NB too
    }
    return fd;
  }
}
