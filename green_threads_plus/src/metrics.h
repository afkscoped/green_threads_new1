#ifndef METRICS_H
#define METRICS_H

#include <stddef.h>
#include <stdint.h>

/* Metric Types */
typedef struct {
  uint64_t context_switches;
  uint64_t active_threads;
  uint64_t scheduler_ticks;
  uint64_t cpu_work;           // Simulated ops
  uint64_t cpu_time_ns;        // Real CPU time (nanoseconds)
  uint64_t latency_buckets[5]; // <1ms, <5ms, <10ms, <50ms, >50ms
} runtime_metrics_t;

typedef struct {
  runtime_metrics_t green;
  runtime_metrics_t pthread;
} system_metrics_t;

extern system_metrics_t global_metrics;

void metrics_init(void);
void metrics_record_switch(int mode);
void metrics_record_tick(int mode);
void metrics_update_active_threads(int mode, int count);
void metrics_get_prometheus(char *buffer, size_t size);
void metrics_to_json(char *buffer, size_t size); // For WebSocket stream
void metrics_record_latency(int mode, double latency_ms);

#endif
