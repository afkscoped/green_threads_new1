#include "green_thread.h"
#include "internal.h"
#include <stdlib.h>

/* Simple mutex for green threads (non-preemptive safety) */
// If non-preemptive, we just need to check a flag.
// If preemptive, we need atomic test-and-set or disabling interrupts/signals.

typedef struct {
  int locked;
  void *owner;
  // queue_t waiting;
} green_mutex_t;

int gt_cond_init(gt_cond_t *c) {
  (void)c;
  // TODO
  return 0;
}

// Implementation of mutex ops handled in dispatchers in green_thread.c mostly?
// Or better to implement here and call from main.
// For now, Green Threads are cooperative, so "locking" is trivial unless we
// yield while holding? If we yield, another thread runs. If it tries to lock,
// it must block. So we need a wait queue.

// Placeholder for full implementation
