#include "internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Scheduler Mode */
static int sched_mode = 0; // 0=RR, 1=Stride
#define LARGE_PASS_VAL 1000000

/* Simple Queue using Intrusive List (Reuse green_thread_t->next) */
static green_thread_t *head = NULL;
static green_thread_t *tail = NULL;

void scheduler_enqueue(void *thread) {
  green_thread_t *t = (green_thread_t *)thread;

  // Clear next to be safe (though usually NULL if coming from outside)
  t->next = NULL;

  if (!head) {
    head = tail = t;
  } else {
    tail->next = t;
    tail = t;
  }
}

void *scheduler_next(void) {
  if (!head)
    return NULL;

  if (sched_mode == 0) {
    // Round Robin: Take head
    green_thread_t *t = head;

    head = head->next;
    if (!head)
      tail = NULL;

    t->next = NULL; // Detach
    return t;
  } else {
    // Stride: Find thread with lowest pass
    green_thread_t *curr = head;
    green_thread_t *min_prev = NULL;
    green_thread_t *min_node = head;
    green_thread_t *prev = NULL;

    uint64_t min_pass = (uint64_t)-1;

    while (curr) {
      if (curr->pass < min_pass) {
        min_pass = curr->pass;
        min_node = curr;
        min_prev = prev;
      }
      prev = curr;
      curr = curr->next;
    }

    // Remove min_node
    if (min_prev) {
      min_prev->next = min_node->next;
      if (min_node == tail)
        tail = min_prev;
    } else {
      head = min_node->next;
      if (!head)
        tail = NULL;
    }

    green_thread_t *t = min_node;
    t->next = NULL; // Detach

    // Update Pass
    // Stride = LargeConstant / tickets
    int tickets = t->tickets > 0 ? t->tickets : 1;
    t->stride = 100000 / tickets;
    t->pass += t->stride;

    return t;
  }
}

void scheduler_schedule(void) {
  // Basic scheduler trigger helper
}

// Internal accessor to change mode
void gt_set_scheduler_type(int type) { sched_mode = type; }

// Update tickets for all threads (Dynamic Priority Change)
void scheduler_update_all_tickets(int tickets) {
  if (tickets <= 0)
    return;

  green_thread_t *curr = head;
  while (curr) {
    curr->tickets = tickets;
    // Reset stride to reflect new priority immediately
    curr->stride = 100000 / tickets;
    curr = curr->next;
  }
}

// Run the scheduler loop until all threads finish
void scheduler_run(void) {
  extern void metrics_update_active_threads(int mode, int count);

  g_workload_running = 1;

  int loop_count = 0;
  while (head != NULL) {
    if (g_shutdown_workload) {
      printf("[Scheduler] Shutdown requested. Cleaning up queue...\n");
      // Drain queue
      while (head) {
        green_thread_t *n = head;
        // Optionally free n->stack here if we had access to stack_free
        // stack_free(n->stack, n->stack_size);
        // free(n);
        head = head->next;
        // In intrusive list, threads are freed elsewhere or assumed managed
        // For this demo, we just drop them.
      }
      tail = NULL;
      break;
    }

    green_thread_t *next = (green_thread_t *)scheduler_next();
    if (!next) {
      break; // No more threads
    }

    // Update metrics
    int active_count = 0;
    green_thread_t *curr = head;
    while (curr) {
      active_count++;
      curr = curr->next;
    }
    if (next)
      active_count++; // Count the running thread too
    metrics_update_active_threads(MODE_GREEN, active_count);

    g_current_thread = next;

    // Switch to the thread (it will yield back)
    extern ucontext_t main_ctx;

    struct timespec ts_start, ts_end;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    swapcontext(&main_ctx, &next->ctx);

    clock_gettime(CLOCK_MONOTONIC, &ts_end);

    // Calculate elapsed time
    uint64_t delta_ns = (ts_end.tv_sec - ts_start.tv_sec) * 1000000000ULL +
                        (ts_end.tv_nsec - ts_start.tv_nsec);

    // Update Thread CPU Time
    next->cpu_time_ns += delta_ns;

    // Update Global Metrics (Simulated + Now Real CPU Time)
    // We reuse the existing "work" metric pipeline but now treat it as "Time
    // units" or add a new one? User requested "Real CPU Usage". Let's add a new
    // metric for this.
    extern void metrics_record_cpu_time(int mode, uint64_t ns);
    metrics_record_cpu_time(MODE_GREEN, delta_ns);

    // Safety check removed to allow infinite run
    loop_count++;
    // if (loop_count > 100000000) break;
  }

  printf("[Scheduler] All green threads finished\n");
  metrics_update_active_threads(MODE_GREEN, 0);
  g_workload_running = 0;
}
