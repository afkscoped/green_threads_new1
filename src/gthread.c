#include "gthread.h"
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
  // Initialize main thread
  g_main_thread.id = 0;
  g_main_thread.state = GTHREAD_RUNNING;
  g_main_thread.stack = NULL; // Main uses system stack
  g_main_thread.tickets = 10;
  g_main_thread.stride = 10000 / 10;
  g_main_thread.pass = 0;
  g_current_thread = &g_main_thread;
}

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

  // Wake up joining threads
  gthread_t *waiter = cur->join_queue;
  while (waiter) {
    gthread_t *next = waiter->next;
    scheduler_enqueue(waiter);
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

  cur->next = t->join_queue;
  t->join_queue = cur;

  scheduler_schedule();

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
  scheduler_enqueue_sleep(cur);
  scheduler_schedule();
}
