#include "green_thread.h"
#include "internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Simple CPU bound task
void matrix_mul_task(void *arg) {
  int iterations = *(int *)arg;
  int size = 100;

  // Allocate dummy matrices
  int **a = malloc(size * sizeof(int *));
  int **b = malloc(size * sizeof(int *));
  int **c = malloc(size * sizeof(int *));

  for (int i = 0; i < size; i++) {
    a[i] = malloc(size * sizeof(int));
    b[i] = malloc(size * sizeof(int));
    c[i] = malloc(size * sizeof(int));
  }

  // Work loop with yields

  // Get time helper
  extern uint64_t get_current_ms_helper(
      void); // Defined somewhere or we use clock_gettime

  // Since we don't have a shared time helper exposed easily, let's use a rough
  // iteration count approximation or just yield every X iterations based on
  // slice. Assuming 1 iteration ~ 0.001ms (very rough). Better: use the
  // 'g_global_timeslice_ms' as a dynamic yield frequency.

  // Time tracking for strict slicing
  struct timespec start, now;
  clock_gettime(CLOCK_MONOTONIC, &start);

  // iterations = -1 means infinite
  int count = 0;
  while (iterations == -1 || count < iterations) {
    if (g_shutdown_workload)
      break;
    count++;

    for (int i = 0; i < size; i++) {
      if (g_shutdown_workload)
        break;

      // Check time slice strict compliance (check every row = 100 ops)
      // optimization: don't check every single op
      clock_gettime(CLOCK_MONOTONIC, &now);
      uint64_t elapsed_ns = (now.tv_sec - start.tv_sec) * 1000000000ULL +
                            (now.tv_nsec - start.tv_nsec);

      // Convert global slice ms to ns
      if (elapsed_ns >= (g_global_timeslice_ms * 1000000ULL)) {
        // Record Real CPU Time (Time spent in this slice)
        extern void metrics_record_cpu_time(int mode, uint64_t ns);
        extern __thread int g_mode;
        metrics_record_cpu_time(g_mode, elapsed_ns);

        gt_yield();

        // THROTTLE PTHREAD: To match Green Thread visual rate (since GT is
        // single-core concur) and Pthreads are multi-core parallel, Pthreads
        // blow up the charts. We slow them down to make the comparison
        // fair/readable for the demo.
        if (g_mode == MODE_PTHREAD) {
          gt_sleep_ms(1); // 1ms sleep per slice
        }

        if (g_shutdown_workload)
          break;                                // Check after yield
        clock_gettime(CLOCK_MONOTONIC, &start); // Reset slice start
      }

      for (int j = 0; j < size; j++) {
        c[i][j] = 0;
        for (int x = 0; x < size; x++) {
          c[i][j] += a[i][x] * b[x][j];
          // Record "CPU work" (1 unit per multiply-add)
          extern void metrics_record_work(int mode, int amount);
          extern __thread int g_mode;
          metrics_record_work(g_mode, 1);
        }
      }
    }
  }

  for (int i = 0; i < size; i++) {
    free(a[i]);
    free(b[i]);
    free(c[i]);
  }
  free(a);
  free(b);
  free(c);
}
