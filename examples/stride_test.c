#include "dashboard.h"
#include "gthread.h"
#include "runtime_stats.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


void cpu_task(void *arg) {
  long id = (long)arg;

  // Simulate CPU work
  for (int i = 0; i < 200; i++) {
    gthread_yield();
    // Simulate some extensive work to make it visible
    volatile int x = 0;
    for (int j = 0; j < 10000; j++)
      x++;
  }
  printf("Thread %ld done.\n", id);
}

int main(void) {
  gthread_init();

  printf("=== Stride Scheduling Demo (Interactive) ===\n");
  printf("This demo visualizes Stride Scheduling with Stacks & CPU usage.\n");
  printf("Open http://localhost:9090 to see the Advanced Dashboard.\n\n");

  int num_threads = 3;
  printf("Enter number of threads (e.g. 3): ");
  scanf("%d", &num_threads);
  if (num_threads < 1)
    num_threads = 1;

  printf("Starting Dashboard on port 9090...\n");
  dashboard_start(9090);

  printf("Creating %d threads with different tickets.\n", num_threads);

  for (int i = 0; i < num_threads; i++) {
    gthread_t *t;
    gthread_create(&t, cpu_task, (void *)(long)(i + 1));

    int tix = 100 - (i * 20); // 100, 80, 60...
    if (tix < 10)
      tix = 10;

    printf("Thread %d: Tickets set to %d\n", i + 1, tix);
    runtime_set_tickets(t->id, tix);
  }

  printf("\nRunning... Press Ctrl+C to stop or wait for threads.\n");
  printf("Go to http://localhost:9090 to dynamically change tickets!\n");

  // Keep main thread alive for dashboard
  while (1) {
    gthread_yield();
    struct timespec ts = {0, 100000000}; // 100ms
    nanosleep(&ts, NULL);

    // Check if all threads done?
    // For now, let it run indefinitely so user can play with dashboard
  }

  return 0;
}
