#include "green_thread.h"
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>


/* Helper to set non-blocking */
int gt_io_set_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1)
    return -1;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/* Wrapper for read that yields if EAGAIN */
ssize_t gt_io_read(int fd, void *buf, size_t count) {
  if (gt_get_mode() == MODE_PTHREAD)
    return read(fd, buf, count);

  while (1) {
    ssize_t ret = read(fd, buf, count);
    if (ret == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // Real implementation needs epoll/poll to register FD and wake up on
        // event For now, simple yield (busy wait roughly)
        gt_yield();
        continue;
      }
    }
    return ret;
  }
}

ssize_t gt_io_write(int fd, const void *buf, size_t count) {
  // Similar logic for write
  if (gt_get_mode() == MODE_PTHREAD)
    return write(fd, buf, count);

  while (1) {
    ssize_t ret = write(fd, buf, count);
    if (ret == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        gt_yield();
        continue;
      }
    }
    return ret;
  }
}
