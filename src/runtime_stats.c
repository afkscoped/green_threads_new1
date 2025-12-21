#include "runtime_stats.h"
#include "gthread.h"
#include "scheduler.h"
#include <stdio.h>
#include <string.h>

// Access internal structures if needed, usually via headers
// We need to iterate gthread_get_all_threads()

extern gthread_t *g_current_thread;

stack_stats_t runtime_get_stack_stats(int tid) {
  stack_stats_t stats = {0};
  gthread_t *curr = gthread_get_all_threads();
  while (curr) {
    if (curr->id == (uint64_t)tid) {
      stats.tid = tid;
      stats.stack_size = curr->stack_size;

      // Calculate usage
      // Stack grows down from (stack + stack_size)
      // Current SP is in ctx.rsp OR if running, we approximate
      uint64_t rsp = curr->ctx.rsp;
      if (curr == g_current_thread) {
        // Approximate for running thread
        uint64_t local_var;
        rsp = (uint64_t)&local_var;
      }

      // If stack is NULL (main thread might be), handle it
      if (curr->stack) {
        uint64_t high_addr = (uint64_t)curr->stack + curr->stack_size;
        if (rsp > (uint64_t)curr->stack && rsp <= high_addr) {
          stats.stack_used = high_addr - rsp;
          stats.stack_remaining = curr->stack_size - stats.stack_used;
        }
      } else {
        // Main thread system stack
        stats.stack_size = 0; // Unknown
      }
      stats.current_sp = (void *)rsp;
      break;
    }
    curr = curr->global_next;
  }
  return stats;
}

// Helper to find FDs - we need to peek into scheduler
// Since scheduler.c is opaque, we can't easily see poll_fds without hacking
// scheduler.c But we can add a 'waiting_fd' field to gthread_main struct for
// easier access? We already have monitor_set_waitfd in monitor.c and monitor
// has it. Let's use the monitor data! Actually, the requirements say "Minimal
// changes to scheduler.c", so I can add int waiting_fd to gthread struct. Let's
// assume we added waiting_fd to gthread_t in include/gthread.h (Phase 11 added
// monitor_id, maybe we add waiting_fd too?) Actually monitor.c HAS this data.
// "io_get_waiting_fd(tid)"
// Accessing monitor's private array?
// No, let's look at `monitor_task_t` in `monitor.c`. It has `wait_fd`.
// But `monitor.c` is static tasks array.
// I can make `monitor_get_task(monitor_id)` public?
// Or I can just check the `runtime_stats` specific fields if I add them.
// Let's add `int waiting_fd` to `gthread_t` for direct access, simpler.

// To implement IO stats properly without coupling to Monitor too tight:
// I will check if gthread has waiting_fd.
// If I can't change gthread.h too much, I'll use monitor.
// But I need to support "live updates".
// Let's follow the plan: Add API to scheduler or gthread.

void runtime_set_tickets(int tid, int tickets) {
  if (tickets < 1)
    tickets = 1;
  gthread_t *curr = gthread_get_all_threads();
  while (curr) {
    if (curr->id == (uint64_t)tid) {
      curr->tickets = tickets;
      curr->stride = 10000 / tickets;
      break;
    }
    curr = curr->global_next;
  }
}

int runtime_get_tickets(int tid) {
  gthread_t *curr = gthread_get_all_threads();
  while (curr) {
    if (curr->id == (uint64_t)tid) {
      return curr->tickets;
    }
    curr = curr->global_next;
  }
  return 0;
}

// IO Stats and Metrics will be filled later after I update scheduler.c to track
// ctx switches For now stubs
io_stats_t runtime_get_io_stats(int tid) {
  io_stats_t stats = {0};
  // To be implemented via lookups
  return stats;
}

runtime_metrics_t runtime_get_metrics(void) {
  runtime_metrics_t m = {0};
  // Count states
  gthread_t *curr = gthread_get_all_threads();
  while (curr) {
    if (curr->state == GTHREAD_READY)
      m.runnable_count++;
    else if (curr->state == GTHREAD_BLOCKED)
      m.waiting_count++; // sleeping is blocked too
    // Distinguish sleep??
    curr = curr->global_next;
  }
  return m;
}
