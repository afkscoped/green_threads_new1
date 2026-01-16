#include "metrics.h"
#include "green_thread.h"
#include <stdio.h>
#include <string.h>

system_metrics_t global_metrics;

void metrics_init(void) { memset(&global_metrics, 0, sizeof(global_metrics)); }

void metrics_record_switch(int mode) {
  if (mode == MODE_GREEN)
    __atomic_fetch_add(&global_metrics.green.context_switches, 1,
                       __ATOMIC_RELAXED);
  else
    __atomic_fetch_add(&global_metrics.pthread.context_switches, 1,
                       __ATOMIC_RELAXED);
}

void metrics_record_tick(int mode) {
  if (mode == MODE_GREEN)
    __atomic_fetch_add(&global_metrics.green.scheduler_ticks, 1,
                       __ATOMIC_RELAXED);
  else
    __atomic_fetch_add(&global_metrics.pthread.scheduler_ticks, 1,
                       __ATOMIC_RELAXED);
}

void metrics_update_active_threads(int mode, int count) {
  if (mode == MODE_GREEN)
    global_metrics.green.active_threads =
        count; // Scheduler sets absolute for Green
  else
    global_metrics.pthread.active_threads = count;
}

void metrics_change_active_threads(int mode, int delta) {
  if (mode == MODE_GREEN) {
    // Green Usually tracked by scheduler, but if mixed:
    __atomic_fetch_add(&global_metrics.green.active_threads, delta,
                       __ATOMIC_RELAXED);
  } else {
    __atomic_fetch_add(&global_metrics.pthread.active_threads, delta,
                       __ATOMIC_RELAXED);
  }
}

void metrics_record_latency(int mode, double latency_ms) {
  runtime_metrics_t *m =
      (mode == MODE_GREEN) ? &global_metrics.green : &global_metrics.pthread;

  // Simple bucket increment (not thread-safe perfect, but acceptable for
  // trends)
  if (latency_ms < 1.0)
    __atomic_fetch_add(&m->latency_buckets[0], 1, __ATOMIC_RELAXED);
  else if (latency_ms < 5.0)
    __atomic_fetch_add(&m->latency_buckets[1], 1, __ATOMIC_RELAXED);
  else if (latency_ms < 10.0)
    __atomic_fetch_add(&m->latency_buckets[2], 1, __ATOMIC_RELAXED);
  else if (latency_ms < 50.0)
    __atomic_fetch_add(&m->latency_buckets[3], 1, __ATOMIC_RELAXED);
  else
    __atomic_fetch_add(&m->latency_buckets[4], 1, __ATOMIC_RELAXED);
}

void metrics_record_work(int mode, int amount) {
  if (mode == MODE_GREEN)
    __atomic_fetch_add(&global_metrics.green.cpu_work, amount,
                       __ATOMIC_RELAXED);
  else
    __atomic_fetch_add(&global_metrics.pthread.cpu_work, amount,
                       __ATOMIC_RELAXED);
}

void metrics_record_cpu_time(int mode, uint64_t ns) {
  if (mode == MODE_GREEN)
    __atomic_fetch_add(&global_metrics.green.cpu_time_ns, ns, __ATOMIC_RELAXED);
  else
    __atomic_fetch_add(&global_metrics.pthread.cpu_time_ns, ns,
                       __ATOMIC_RELAXED);
}

void metrics_get_prometheus(char *buffer, size_t size) {
  snprintf(
      buffer, size,
      "# HELP green_threads_context_switch_total Total context switches\n"
      "# TYPE green_threads_context_switch_total counter\n"
      "green_threads_context_switch_total{mode=\"green\"} %llu\n"
      "green_threads_context_switch_total{mode=\"pthread\"} %llu\n"

      "# HELP green_threads_active_threads Current active thread count\n"
      "# TYPE green_threads_active_threads gauge\n"
      "green_threads_active_threads{mode=\"green\"} %llu\n"
      "green_threads_active_threads{mode=\"pthread\"} %llu\n"

      "# HELP green_threads_scheduler_ticks_total Total scheduler ticks\n"
      "# TYPE green_threads_scheduler_ticks_total counter\n"
      "green_threads_scheduler_ticks_total{mode=\"green\"} %llu\n"
      "green_threads_scheduler_ticks_total{mode=\"pthread\"} %llu\n"

      "# HELP green_threads_cpu_work_total Total simulated CPU operations\n"
      "# TYPE green_threads_cpu_work_total counter\n"
      "green_threads_cpu_work_total{mode=\"green\"} %llu\n"
      "green_threads_cpu_work_total{mode=\"pthread\"} %llu\n"

      "# HELP green_threads_cpu_seconds_total Total Real CPU time seconds\n"
      "# TYPE green_threads_cpu_seconds_total counter\n"
      "green_threads_cpu_seconds_total{mode=\"green\"} %.6f\n"
      "green_threads_cpu_seconds_total{mode=\"pthread\"} %.6f\n",

      // Arguments in order: switches, active, ticks, work, time
      (unsigned long long)global_metrics.green.context_switches,
      (unsigned long long)global_metrics.pthread.context_switches,
      (unsigned long long)global_metrics.green.active_threads,
      (unsigned long long)global_metrics.pthread.active_threads,
      (unsigned long long)global_metrics.green.scheduler_ticks,
      (unsigned long long)global_metrics.pthread.scheduler_ticks,
      (unsigned long long)global_metrics.green.cpu_work,
      (unsigned long long)global_metrics.pthread.cpu_work,
      ((double)global_metrics.green.cpu_time_ns / 1.0e9),
      ((double)global_metrics.pthread.cpu_time_ns / 1.0e9));
}

void metrics_to_json(char *buffer, size_t size) {
  snprintf(buffer, size,
           "{\"type\":\"metrics\",\"green\":{\"switches\":%llu,\"active\":%llu,"
           "\"ticks\":%llu,\"work\":%llu,\"cpu_time\":%llu},\"pthread\":{"
           "\"switches\":%llu,"
           "\"active\":%llu,"
           "\"ticks\":%llu,\"work\":%llu,\"cpu_time\":%llu}}",
           (unsigned long long)global_metrics.green.context_switches,
           (unsigned long long)global_metrics.green.active_threads,
           (unsigned long long)global_metrics.green.scheduler_ticks,
           (unsigned long long)global_metrics.green.cpu_work,
           (unsigned long long)global_metrics.green.cpu_time_ns, // Nanoseconds
           (unsigned long long)global_metrics.pthread.context_switches,
           (unsigned long long)global_metrics.pthread.active_threads,
           (unsigned long long)global_metrics.pthread.scheduler_ticks,
           (unsigned long long)global_metrics.pthread.cpu_work,
           (unsigned long long)global_metrics.pthread.cpu_time_ns // Nanoseconds
  );
}
