#include "scheduler.h"
#include "gthread.h"
#include <stdio.h>
#include <stdlib.h>

// Stride Scheduling Constants
#define STRIDE_CONSTANT 10000

/* Min-Heap Implementation for Ready Queue */
#define HEAP_CAPACITY 1024
static gthread_t *ready_heap[HEAP_CAPACITY];
static int heap_size = 0;

gthread_t *g_current_thread = NULL;
gthread_t g_main_thread;

/* Zombie thread to be freed */
static gthread_t *g_zombie_thread = NULL;

/* External assembly function */
extern void gthread_switch(gthread_ctx_t *new_ctx, gthread_ctx_t *old_ctx);

void scheduler_enqueue(gthread_t *t) {
  if (heap_size >= HEAP_CAPACITY) {
    fprintf(stderr, "Scheduler: Heap overflow\n");
    return;
  }

  t->state = GTHREAD_READY;

  // Insert at end
  int i = heap_size++;
  ready_heap[i] = t;

  // Bubble up
  while (i > 0) {
    int parent = (i - 1) / 2;
    if (ready_heap[parent]->pass <= ready_heap[i]->pass) {
      break;
    }
    // Swap
    gthread_t *temp = ready_heap[parent];
    ready_heap[parent] = ready_heap[i];
    ready_heap[i] = temp;
    i = parent;
  }
}

gthread_t *scheduler_dequeue(void) {
  if (heap_size == 0)
    return NULL;

  gthread_t *min = ready_heap[0];

  // Move last to root
  ready_heap[0] = ready_heap[--heap_size];

  // Bubble down
  int i = 0;
  while (1) {
    int left = 2 * i + 1;
    int right = 2 * i + 2;
    int smallest = i;

    if (left < heap_size &&
        ready_heap[left]->pass < ready_heap[smallest]->pass) {
      smallest = left;
    }
    if (right < heap_size &&
        ready_heap[right]->pass < ready_heap[smallest]->pass) {
      smallest = right;
    }

    if (smallest == i)
      break;

    gthread_t *temp = ready_heap[i];
    ready_heap[i] = ready_heap[smallest];
    ready_heap[smallest] = temp;
    i = smallest;
  }

  return min;
}

static void free_zombie(void) {
  if (g_zombie_thread) {
    if (g_zombie_thread->stack) {
      free(g_zombie_thread->stack);
      g_zombie_thread->stack = NULL;
    }
    // We DO NOT free the struct because it is part of the global list used by
    // Dashboard. In a real system we would unlink it safely.
    // free(g_zombie_thread);
    g_zombie_thread = NULL;
  }
}

/* Sleep Queue (Unsorted List) */
static gthread_t *sleep_list = NULL;

void scheduler_enqueue_sleep(gthread_t *t) {
  t->state = GTHREAD_BLOCKED;
  t->next = sleep_list;
  sleep_list = t;
}

#include <time.h>
static uint64_t get_time_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

static void check_timers(void) {
  if (!sleep_list)
    return;

  uint64_t now = get_time_ms();

  gthread_t *prev = NULL;
  gthread_t *curr = sleep_list;

  while (curr) {
    if (curr->wake_time_ms <= now) {
      // Wake up
      gthread_t *next = curr->next;

      if (prev)
        prev->next = next;
      else
        sleep_list = next;

      scheduler_enqueue(curr);
      curr = next;
    } else {
      prev = curr;
      curr = curr->next;
    }
  }
}

#include <poll.h>

#define MAX_POLL_FDS 128
static struct pollfd poll_fds[MAX_POLL_FDS];
static gthread_t *poll_threads[MAX_POLL_FDS];
static int poll_count = 0;

void scheduler_register_io_wait(int fd, int events) {
  if (poll_count >= MAX_POLL_FDS)
    return;

  poll_fds[poll_count].fd = fd;
  poll_fds[poll_count].events = events;
  poll_fds[poll_count].revents = 0;

  poll_threads[poll_count] = g_current_thread;
  g_current_thread->state = GTHREAD_BLOCKED;
  g_current_thread->waiting_fd = fd; // Phase 13

  poll_count++;
  scheduler_schedule();
}

static void check_io(int timeout_ms) {
  if (poll_count == 0 && timeout_ms > 0) {
    usleep(timeout_ms * 1000);
    return;
  }
  if (poll_count == 0 && timeout_ms <= 0)
    return;

  int ret = poll(poll_fds, poll_count, timeout_ms);
  if (ret > 0) {
    for (int i = 0; i < poll_count; i++) {
      if (poll_fds[i].revents) {
        gthread_t *t = poll_threads[i];
        t->waiting_fd = -1; // Phase 13
        scheduler_enqueue(t);

        // Remove
        poll_fds[i] = poll_fds[poll_count - 1];
        poll_threads[i] = poll_threads[poll_count - 1];
        poll_count--;
        i--;
      }
    }
  }
}

void scheduler_schedule(void) {
  // Try to clear IO first
  check_io(0);
  check_timers();

  gthread_t *prev = g_current_thread;
  gthread_t *next = scheduler_dequeue();

  while (!next) {
    // Logic: If no threads, we wait.
    // Calc timeout:
    int timeout = -1;
    if (sleep_list) {
      uint64_t now = get_time_ms();
      // Find min wake time
      gthread_t *curr = sleep_list;
      uint64_t min_wake = (uint64_t)-1;
      while (curr) {
        if (curr->wake_time_ms < min_wake)
          min_wake = curr->wake_time_ms;
        curr = curr->next;
      }
      if (min_wake > now)
        timeout = (int)(min_wake - now);
      else
        timeout = 0;
    }

    // If no IO and no timers, and Main is blocked (or we are not main), check
    // deadlock
    if (poll_count == 0 && timeout == -1) {
      if (prev != &g_main_thread && prev->state != GTHREAD_TERMINATED &&
          prev->state != GTHREAD_BLOCKED) {
        return; // Keep running prev
      }
      if (prev != &g_main_thread) {
        next = &g_main_thread; // Switch to Main
        if (g_main_thread.state != GTHREAD_BLOCKED)
          break;
      }
      // Deadlock if Main is blocked or we are main and blocked
      if (g_main_thread.state == GTHREAD_BLOCKED) {
        fprintf(stderr, "Scheduler: Deadlock (Main blocked, no IO/Timers)\n");
        exit(1);
      }
    }

    // Allow interruptible wait
    check_io(timeout == -1 ? 100
                           : timeout); // 100ms default if no timers but IO
    check_timers();
    next = scheduler_dequeue();

    if (next)
      break;

    // If still nothing and poll_count 0 and timeout -1? loop or exit logic
    // above handles deadlock
    if (poll_count == 0 && timeout == -1) {
      break; // Fallback to main loop logic
    }
  }

  // Check if prev is terminating
  if (prev->state == GTHREAD_TERMINATED && prev != &g_main_thread) {
    free_zombie(); // Free any previous zombie
    g_zombie_thread = prev;
  }

  // Update Pass
  next->pass += next->stride;

  g_current_thread = next;
  next->state = GTHREAD_RUNNING;

  if (prev != next) {
    gthread_switch(&next->ctx, &prev->ctx);
  }
}
