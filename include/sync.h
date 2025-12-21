#ifndef SYNC_H
#define SYNC_H

#include "gthread.h"

/* Mutex */
typedef struct {
  int locked;
  gthread_t *wait_queue; // Queue of blocked threads
} gmutex_t;

void gmutex_init(gmutex_t *m);
void gmutex_lock(gmutex_t *m);
void gmutex_unlock(gmutex_t *m);

/* Condition Variable */
typedef struct {
  gthread_t *wait_queue;
} gcond_t;

void gcond_init(gcond_t *c);
void gcond_wait(gcond_t *c, gmutex_t *m);
void gcond_signal(gcond_t *c);
void gcond_broadcast(gcond_t *c);

#endif
