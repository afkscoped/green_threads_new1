#ifndef RUNTIME_STATS_H
#define RUNTIME_STATS_H

#include <stddef.h>

// Stack Stats
typedef struct {
  int tid;
  size_t stack_size;
  size_t stack_used;
  size_t stack_remaining;
  void *current_sp;
} stack_stats_t;

// IO Stats
typedef struct {
  int tid;
  int waiting_fd;
  long wait_time_ms; // Duration waiting
} io_stats_t;

// Runtime Metrics
typedef struct {
  int runnable_count;
  int sleeping_count;
  int waiting_count;
  long ctx_switches_per_sec;
  long scheduler_ticks;
} runtime_metrics_t;

// APIs
stack_stats_t runtime_get_stack_stats(int tid);
io_stats_t runtime_get_io_stats(int tid);
runtime_metrics_t runtime_get_metrics(void);
void runtime_set_tickets(int tid, int tickets);
int runtime_get_tickets(int tid);

#endif
