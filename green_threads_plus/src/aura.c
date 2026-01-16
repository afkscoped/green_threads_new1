#include "aura.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static uint64_t __attribute__((unused)) get_current_ms() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

void aura_init(void) {
  printf("[AURA] Initialized Adaptive Runtime Analyzer\n");
}

void aura_on_thread_start(green_thread_t *t) {
  (void)t;
  // Track new thread specific stats if we expand AURA structs
}

void aura_on_schedule(green_thread_t *t) {
  // In real implementation we'd track last_run_timestamp in 't'
  // For now we just print/log validation
  // t->last_run = get_current_ms();
  (void)t;
}

#include "metrics.h"

// External access to global metrics
extern system_metrics_t
    global_metrics; // we need to remove static or add getter in metrics.c
// Check metrics.c ... it is static. I should make it non-static or add a
// getter. Let's assume we fix metrics.c to remove 'static' or provide a getter.

// Actually, let's just make metrics.c global_metrics non-static for this demo.
// (I will do that in next step)

void aura_tick(void) {
  // 1. Check Fairness (Green vs Pthread)
  // If active count is similar but switch count differs by > 50%, flag it.

  uint64_t g_switches =
      __atomic_load_n(&global_metrics.green.context_switches, __ATOMIC_RELAXED);
  uint64_t p_switches = __atomic_load_n(
      &global_metrics.pthread.context_switches, __ATOMIC_RELAXED);

  uint64_t g_active = global_metrics.green.active_threads;
  uint64_t p_active = global_metrics.pthread.active_threads;

  if (g_active > 0 && p_active > 0) {
    double g_ratio = (double)g_switches / g_active;
    double p_ratio = (double)p_switches / p_active;

    // Heuristic: If one mode is switching 10x faster, potential issue
    // NOTE: Disabled because Pthreads (Multi-core) naturally switch FASTER than
    // Green (Single-core) and this floods the logs, potentially crashing the
    // container IO.
    /*
    if (g_ratio < (p_ratio * 0.1)) {
      printf("[AURA] ALERT: Green Threads Starvation/Stall Detected! (Green "
             "Ratio: %.2f, Pthread Ratio: %.2f)\n",
             g_ratio, p_ratio);
    } else if (p_ratio < (g_ratio * 0.1)) {
      printf("[AURA] ALERT: Pthread Starvation Detected! (Green Ratio: %.2f, "
             "Pthread Ratio: %.2f)\n",
             g_ratio, p_ratio);
    }
    */
  }
}
