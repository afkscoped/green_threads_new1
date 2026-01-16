#define _GNU_SOURCE
#include "green_thread.h"
#include "internal.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct {
  gt_fn fn;
  void *arg;
  gt_tid tid;
} pt_arg_t;

static void *pt_wrapper(void *arg) {
  extern void metrics_change_active_threads(int mode, int delta);
  metrics_change_active_threads(MODE_PTHREAD, 1);

  pt_arg_t *a = (pt_arg_t *)arg;
  a->fn(a->arg);
  free(a);

  metrics_change_active_threads(MODE_PTHREAD, -1);
  return NULL;
}

static gt_tid pt_next_id = 1;
// In a real generic map, we'd map pthread_t to gt_tid.
// For this simple adapter, we might not track all metadata perfectly unless
// needed.

int pt_init(void) {
  return 0; // nothing specific for pthreads usually
}

gt_tid pt_create(gt_fn fn, void *arg, size_t stack_size, int priority) {
  (void)priority;
  pthread_t thread;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  if (stack_size > 0)
    pthread_attr_setstacksize(&attr, stack_size);

  pt_arg_t *a = malloc(sizeof(pt_arg_t));
  a->fn = fn;
  a->arg = arg;
  a->tid = __sync_fetch_and_add(&pt_next_id, 1);

  if (pthread_create(&thread, &attr, pt_wrapper, a) != 0) {
    free(a);
    return -1;
  }
  pthread_attr_destroy(&attr);
  // Ideally we store 'thread' mapped to 'tid' to join later.
  // For now we assume detached or handle leaks for simplicity until phase 2.
  // TODO: Maintain a list of (tid, pthread_t) for join.
  return a->tid;
}

void pt_yield(void) { sched_yield(); }

void pt_join(gt_tid tid) {
  (void)tid;
  // TODO: look up pthread_t from tid and join
  // This requires a global list/map protected by a mutex.
}

void pt_exit(void) { pthread_exit(NULL); }

void pt_sleep_ms(int ms) { usleep(ms * 1000); }

// Mutex adapter
int pt_mutex_init(gt_mutex_t *m) {
  m->internal = malloc(sizeof(pthread_mutex_t));
  if (pthread_mutex_init((pthread_mutex_t *)m->internal, NULL) != 0)
    return -1;
  return 0;
}

int pt_mutex_lock(gt_mutex_t *m) {
  return pthread_mutex_lock((pthread_mutex_t *)m->internal);
}

int pt_mutex_unlock(gt_mutex_t *m) {
  return pthread_mutex_unlock((pthread_mutex_t *)m->internal);
}
