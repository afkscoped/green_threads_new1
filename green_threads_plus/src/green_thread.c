#define _GNU_SOURCE
#include "green_thread.h"
#include "internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>

__thread int g_mode = MODE_GREEN;
static gt_tid next_tid = 1;
__thread green_thread_t *g_current_thread = NULL;
ucontext_t main_ctx;

/* Global Control Flags Definition */
volatile int g_shutdown_workload = 0;
volatile int g_workload_running = 0;
volatile int g_any_workload_running = 0;

/* Trampoline for green threads */
static void green_thread_entry(void) {
  if (g_current_thread && g_current_thread->entry) {
    g_current_thread->entry(g_current_thread->arg);
  }
  gt_exit();
}

int gt_init(int mode) {
  g_mode = mode;
  if (g_mode == MODE_PTHREAD) {
    return pt_init();
  }

  // Initialize Main Thread as a Green Thread
  // We don't allocate a stack for main, it uses the OS stack.
  // But we need a struct to represent it.
  green_thread_t *main_thread = calloc(1, sizeof(green_thread_t));
  main_thread->id = 0;
  main_thread->state = 1; // RUNNING
  g_current_thread = main_thread;

  // Start Control Server (in background)
  // This allows the Dashboard to connect even if running minimal CLI
  extern void start_http_server(int port);
  start_http_server(8081);

  return 0;
}

gt_tid gt_create(gt_fn fn, void *arg, size_t stack_size, int priority) {
  if (g_mode == MODE_PTHREAD) {
    return pt_create(fn, arg, stack_size, priority);
  }

  green_thread_t *t = calloc(1, sizeof(green_thread_t));
  if (!t)
    return -1;

  t->id = next_tid++;
  t->entry = fn;
  t->arg = arg;
  t->stack_size = stack_size ? stack_size : (64 * 1024);
  t->stack = stack_alloc(t->stack_size);
  t->priority = priority;
  t->tickets = g_default_tickets;

  if (getcontext(&t->ctx) == -1) {
    perror("getcontext");
    exit(1);
  }

  t->ctx.uc_stack.ss_sp = t->stack;
  t->ctx.uc_stack.ss_size = t->stack_size;
  t->ctx.uc_link = &main_ctx; // Fallback? Ideally should call gt_exit

  makecontext(&t->ctx, green_thread_entry, 0);

  // Enqueue
  scheduler_enqueue(t);

  return t->id;
}

void gt_yield(void) {
  if (g_mode == MODE_PTHREAD) {
    extern void metrics_record_switch(int mode);
    metrics_record_switch(MODE_PTHREAD);
    pt_yield();
    return;
  }

  green_thread_t *prev = g_current_thread;
  scheduler_enqueue(prev);

  green_thread_t *next = (green_thread_t *)scheduler_next();
  if (next && next != prev) {
    g_current_thread = next;
    extern void metrics_record_switch(int mode);
    metrics_record_switch(MODE_GREEN);
    swapcontext(&prev->ctx, &next->ctx);
  }
}

void gt_exit(void) {
  if (g_mode == MODE_PTHREAD) {
    pt_exit();
    return;
  }

  // Simple exit: mark terminated, schedule next
  // Note: We are running on this thread's stack. We can't free it yet easily if
  // we are on it. In a real system we'd switch to a scheduler context or a
  // "cleanup" thread. For simplicity, we assume scheduler_next() gives us
  // someone else, and we never return here.

  // dead->state = TERMINATED;

  // We should probably switch to main_ctx or something safe if no threads left?
  // For now, simple schedule:
  green_thread_t *next = (green_thread_t *)scheduler_next();
  if (next) {
    g_current_thread = next;
    // setcontext because we don't want to save current dead context (or maybe
    // we do for debug)
    setcontext(&next->ctx);
  } else {
    // No threads left? Return to main or exit?
    // If we are main, we shouldn't exit this way via gt_exit calls usually, but
    // valid.
    exit(0);
  }
}

void gt_join(gt_tid tid) {
  if (g_mode == MODE_PTHREAD) {
    pt_join(tid);
    return;
  }
  // Phase 2 implementation of join (blocking current until tid dead)
}

void gt_sleep_ms(int ms) {
  if (g_mode == MODE_PTHREAD) {
    pt_sleep_ms(ms);
    return;
  }
  // Implementation deferred to timers.c using signals or just busy wait for
  // now? Prompt asks for timers.c timers_sleep(ms);
  usleep(ms * 1000); // Temporary blocking hack for Phase 1
}

int gt_get_mode(void) { return g_mode; }

// Mutex Dispatchers
int gt_mutex_init(gt_mutex_t *m) {
  if (g_mode == MODE_PTHREAD)
    return pt_mutex_init(m);
  // Green mutex init
  return 0;
}
int gt_mutex_lock(gt_mutex_t *m) {
  if (g_mode == MODE_PTHREAD)
    return pt_mutex_lock(m);
  return 0;
}
int gt_mutex_unlock(gt_mutex_t *m) {
  if (g_mode == MODE_PTHREAD)
    return pt_mutex_unlock(m);
  return 0;
}

/* Scheduler config dispatch */
int g_global_timeslice_ms = 10;
int g_default_tickets = 10;

void gt_set_stride_tickets(gt_tid tid, int tickets) {
  (void)tid;
  // Set default for new threads
  if (tickets > 0)
    g_default_tickets = tickets;

  // Also update ALL existing threads for immediate feedback
  // This interacts with the scheduler queue directly
  extern void scheduler_update_all_tickets(int tickets);
  scheduler_update_all_tickets(tickets);
}

void gt_set_global_timeslice_ms(int ms) {
  if (ms > 0)
    g_global_timeslice_ms = ms;
}
