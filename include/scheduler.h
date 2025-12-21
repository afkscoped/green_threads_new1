#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "gthread.h"

/* Global scheduler state */
extern gthread_t *g_current_thread;
extern gthread_t *g_ready_queue_head;
extern gthread_t *g_ready_queue_tail;
extern gthread_t g_main_thread;

/* Internal functions */
void scheduler_schedule(void);
void scheduler_enqueue(gthread_t *t);
void scheduler_enqueue_sleep(gthread_t *t);
void scheduler_register_io_wait(int fd, int events);

#endif
