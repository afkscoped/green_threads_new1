#include "green_thread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void matrix_worker(void *arg) {
  int id = *(int *)arg;
  free(arg);

  printf("Worker %d started (Continuous Mode)\n", id);

  while (1) {
    // Simulate CPU work
    for (volatile int k = 0; k < 1000000; k++)
      ;

    gt_yield();
    gt_sleep_ms(50); // High frequency switching
  }
}

int main(int argc, char **argv) {
  int mode = MODE_GREEN;
  if (argc > 1 && strcmp(argv[1], "pthread") == 0) {
    mode = MODE_PTHREAD;
  }

  printf("Initializing GreenThreads++ in %s mode...\n",
         mode == MODE_GREEN ? "GREEN" : "PTHREAD");
  gt_init(mode);

  // Spawn some workers
  for (int i = 0; i < 4; i++) {
    int *arg = malloc(sizeof(int));
    *arg = i;
    gt_create(matrix_worker, arg, 0, 0);
  }

  printf("Main thread yielding...\n");
  gt_yield();

  printf("GreenThreads++ Server Running (Press Ctrl+C to stop)...\n");
  while (1) {
    gt_sleep_ms(1000);
  }

  return 0;
}
