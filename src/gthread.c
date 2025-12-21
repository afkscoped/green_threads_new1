#include "gthread.h"
#include "monitor.h"
#include "scheduler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#define DEFAULT_STACK_SIZE (64 * 1024)

extern void gthread_trampoline(void);
extern void gthread_switch(gthread_ctx_t *new_ctx, gthread_ctx_t *old_ctx);

static uint64_t next_tid = 1;

void gthread_init(void) {
  monitor_init();
  // Initialize main thread
  g_main_thread.id = 0;
  g_main_thread.state = GTHREAD_RUNNING;
  g_main_thread.stack = NULL; // Main uses system stack
  g_main_thread.tickets = 10;
  g_main_thread.stride = 10000 / 10;
  g_main_thread.pass = 0;
  g_main_thread.pass = 0;
  g_main_thread.monitor_id = monitor_register("MAIN");
  monitor_update_state(g_main_thread.monitor_id, TASK_RUNNABLE);

  // Dashboard #2 Global List
  g_main_thread.global_next = NULL;
  // We need to export this or have an accessor. Making it extern for now in
  // header? Or just a static here and an accessor function? Accessor is
  // cleaner. I will add a helper to iterate or get head.
  g_current_thread = &g_main_thread;
}

// Global list head
gthread_t *g_all_threads = &g_main_thread;

gthread_t *gthread_get_all_threads(void) { return g_all_threads; }

int gthread_create(gthread_t **t, void (*fn)(void *), void *arg) {
  if (!g_current_thread)
    gthread_init();

  gthread_t *thread = (gthread_t *)malloc(sizeof(gthread_t));
  if (!thread)
    return -1;

  // Allocate stack
  thread->stack_size = DEFAULT_STACK_SIZE;
  thread->stack = malloc(thread->stack_size);
  if (!thread->stack) {
    free(thread);
    return -1;
  }

  thread->id = next_tid++;
  thread->entry = fn;
  thread->arg = arg;
  thread->state = GTHREAD_NEW;
  thread->tickets = 10; // Default tickets
  thread->pass = 0;
  thread->stride = 10000; // arbitrary constant / tickets

  // Monitor
  thread->monitor_id = monitor_register("GTHREAD");
  monitor_update_state(thread->monitor_id, TASK_RUNNABLE);

  // Add to global list
  thread->global_next = g_all_threads;
  g_all_threads = thread;

  // Setup Context
  // Stack grows down. Top is stack + size.
  // We need 16-byte alignment for ABI.
  uint64_t *stack_top =
      (uint64_t *)((char *)thread->stack + thread->stack_size);
  // Align
  stack_top = (uint64_t *)(((uint64_t)stack_top) & ~0xF);

  thread->ctx.rsp = (uint64_t)stack_top;
  thread->ctx.rip = (uint64_t)gthread_trampoline;

  // Set Callee Saved Registers to pass args to trampoline
  thread->ctx.r12 = (uint64_t)fn;
  thread->ctx.r13 = (uint64_t)arg;

  scheduler_enqueue(thread);
  *t = thread;
  return 0;
}

void gthread_exit(void) {
  gthread_t *cur = g_current_thread;
  cur->state = GTHREAD_TERMINATED;
  monitor_mark_done(cur->monitor_id);

  // Wake up joining threads
  gthread_t *waiter = cur->join_queue;
  while (waiter) {
    gthread_t *next = waiter->next;
    scheduler_enqueue(waiter);
    monitor_update_state(waiter->monitor_id, TASK_RUNNABLE);
    waiter = next;
  }
  cur->join_queue = NULL;

  // Schedule next
  scheduler_schedule();
}

void gthread_yield(void) {
  if (g_current_thread) {
    scheduler_enqueue(g_current_thread);
    scheduler_schedule();
  }
}

int gthread_join(gthread_t *t, void **retval) {
  if (!t)
    return -1;
  if (t->state == GTHREAD_TERMINATED) {
    if (retval)
      *retval = t->retval;
    return 0;
  }

  // Add self to t's join queue
  gthread_t *cur = g_current_thread;
  cur->state = GTHREAD_BLOCKED;
  monitor_update_state(cur->monitor_id, TASK_WAITING);

  cur->next = t->join_queue;
  t->join_queue = cur;

  scheduler_schedule();

  monitor_update_state(cur->monitor_id, TASK_RUNNABLE);

  if (retval)
    *retval = t->retval;
  return 0;
}

#include <time.h>
static uint64_t get_time_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

void gthread_sleep(uint64_t ms) {
  gthread_t *cur = g_current_thread;
  cur->wake_time_ms = get_time_ms() + ms;

  monitor_update_state(cur->monitor_id, TASK_SLEEPING);
  monitor_set_wake(cur->monitor_id, cur->wake_time_ms);

  scheduler_enqueue_sleep(cur);
  scheduler_schedule();

  monitor_update_state(cur->monitor_id, TASK_RUNNABLE);
  monitor_set_wake(cur->monitor_id, -1);
}
