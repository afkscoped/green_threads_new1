#ifndef INTERNAL_H
#define INTERNAL_H

#include "green_thread.h"
#include <ucontext.h>

/* Global Mode */
extern __thread int g_mode;

/* Pthread Adapter Functions */
int pt_init(void);
gt_tid pt_create(gt_fn fn, void *arg, size_t stack_size, int priority);
void pt_yield(void);
void pt_join(gt_tid tid);
void pt_exit(void);
void pt_sleep_ms(int ms);
int pt_mutex_init(gt_mutex_t *m);
int pt_mutex_lock(gt_mutex_t *m);
int pt_mutex_unlock(gt_mutex_t *m);

/* Scheduler Interface (Internal) */
void scheduler_enqueue(void *thread);
void scheduler_schedule(void);
void *scheduler_next(void);
void scheduler_run(void);
void scheduler_update_all_tickets(int tickets);

/* Global Control Flags */
extern volatile int g_shutdown_workload;
extern volatile int g_workload_running;     // Specific to Green Scheduler
extern volatile int g_any_workload_running; // Covers both Green and Pthreads
                                            // (managed by spawn_helper)
extern int g_global_timeslice_ms;

/* Stack Allocation */
void *stack_alloc(size_t size);
void stack_free(void *stack, size_t size);

/* Internal Green Thread Structure */
typedef struct green_thread {
  gt_tid id;
  ucontext_t ctx;
  void *stack;
  size_t stack_size;

  gt_fn entry;
  void *arg;

  int state; // 0=READY, 1=RUNNING, etc.
  int priority;
  int tickets;
  uint64_t pass;
  uint64_t stride;

  uint64_t cpu_time_ns; // Real execution time

  struct green_thread *next;
} green_thread_t;

extern __thread green_thread_t *g_current_thread;

#endif // INTERNAL_H
