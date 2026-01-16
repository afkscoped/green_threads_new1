#define _GNU_SOURCE
#include "green_thread.h"
#include "internal.h"
#include "metrics.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Worker Function for Demos */
void (*g_worker_fn)(void *) = NULL;
void *g_worker_arg = NULL;

/* Pthread Worker Wrapper */
void *pthread_worker_entry(void *arg) {
  g_mode = MODE_PTHREAD;
  extern void metrics_change_active_threads(int mode, int delta);
  metrics_change_active_threads(MODE_PTHREAD, 1);

  if (g_worker_fn)
    g_worker_fn(arg);

  metrics_change_active_threads(MODE_PTHREAD, -1);
  return NULL;
}

/* Green Worker Wrapper */
void green_worker_entry(void *arg) {
  g_mode = MODE_GREEN; // Ensure metrics are attributed to green threads
  if (g_worker_fn)
    g_worker_fn(arg);
}

/* Green Scheduler Wrapper (runs in separate pthread) */
static void *green_scheduler_wrapper(void *data) {
  int num_green = *(int *)data;
  free(data);

  // Initialize green thread runtime in this pthread
  gt_init(MODE_GREEN);

  // Create green threads
  for (int i = 0; i < num_green; i++) {
    gt_create(green_worker_entry, g_worker_arg, 0, 0);
  }

  // Run the scheduler loop
  extern void scheduler_run(void);
  scheduler_run();

  return NULL;
}

/* Dual Mode Orchestrator */
void dual_run_workload(void (*worker)(void *), void *arg, int num_green,
                       int num_pthread) {
  g_worker_fn = worker;
  g_worker_arg = arg;

  printf("[DualRun] Spawning %d Green Threads and %d Pthreads...\n", num_green,
         num_pthread);

  // 1. Spawn Pthreads
  pthread_t *pthreads = calloc(num_pthread, sizeof(pthread_t));
  for (int i = 0; i < num_pthread; i++) {
    pthread_create(&pthreads[i], NULL, pthread_worker_entry, arg);
    metrics_update_active_threads(MODE_PTHREAD, i + 1);
  }

  // 2. Spawn Green Threads in a background pthread
  pthread_t green_sched_thread;
  int green_spawned = 0;

  if (num_green > 0) {
    int *green_count = malloc(sizeof(int));
    *green_count = num_green;
    pthread_create(&green_sched_thread, NULL, green_scheduler_wrapper,
                   green_count);
    green_spawned = 1;
  }

  // Wait for Pthreads
  for (int i = 0; i < num_pthread; i++) {
    pthread_join(pthreads[i], NULL);
  }

  // KEY FIX: Wait for Green Scheduler (it runs until Stop command)
  // Previously we detached, so spawn_helper returned immediately for Green-only
  // workloads, causing overlapping spawns and chaos.
  if (green_spawned) {
    pthread_join(green_sched_thread, NULL);
  }

  free(pthreads);
  printf("[DualRun] Workload Complete.\n");
}
