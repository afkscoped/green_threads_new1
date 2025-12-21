#include "dashboard.h"
#include "gthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


void recursive_task(void *arg) {
  long depth = (long)arg;
  char buffer[1024]; // Consume 1KB stack per frame

  // Touch buffer to ensure allocation
  buffer[0] = (char)depth;

  gthread_yield();
  struct timespec ts = {0, 50000000}; // 50ms slow down
  nanosleep(&ts, NULL);

  if (depth > 0) {
    // printf("Thread %lu at depth %ld (Stack ~%ld KB)\n", gthread_get_id(),
    // depth, (64 - depth));
    recursive_task((void *)(depth - 1));
  } else {
    printf("Thread %lu reached max depth!\n", gthread_get_id());
    // Hold at max depth to show on dashboard
    for (int i = 0; i < 100; i++) {
      gthread_yield();
      nanosleep(&ts, NULL);
    }
  }
}

int main(void) {
  gthread_init();

  printf("=== Stack Management Demo (Interactive) ===\n");
  printf("Visualizes stack growth. Open http://localhost:9090/ \n");

  dashboard_start(9090);

  int depth;
  printf("Enter recursion depth (1KB per frame, max ~60): ");
  scanf("%d", &depth);
  if (depth < 1)
    depth = 1;
  if (depth > 62) {
    printf("Warning: Depth > 62 might overflow default 64KB stack.\n");
  }

  gthread_t *t;
  gthread_create(&t, recursive_task, (void *)(long)depth);

  printf(
      "Thread created. Watch the 'Stack Usage' bar turn RED on dashboard!\n");

  while (1) {
    gthread_yield();
    sleep(1);
  }
  return 0;
}
