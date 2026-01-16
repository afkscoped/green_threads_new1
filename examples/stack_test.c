#include "gthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Recursive function to eat stack
void deep_recursion(int depth) {
  char buffer[1024];                  // 1KB per frame
  sprintf(buffer, "Depth %d", depth); // Use it to prevent optimization

  if (depth > 0) {
    if (depth % 10 == 0) {
      printf("Stack test: Depth %d\n", depth);
      gthread_yield();
    }
    deep_recursion(depth - 1);
  } else {
    printf("Reached max depth!\n");
  }
}

void thread_func(void *arg) {
  long depth = (long)arg;
  printf("Thread starting recursion to depth %ld...\n", depth);
  deep_recursion((int)depth);
  printf("Thread finished recursion\n");
}

int main(void) {
  gthread_init();

  int depth;
  printf("Enter recursion depth (1-60): ");
  if (scanf("%d", &depth) != 1)
    depth = 20;
  if (depth < 1)
    depth = 1;
  if (depth > 60) {
    printf("Warning: Depth > 60 might overflow 64KB stack.\n");
  }

  gthread_t *t;
  gthread_create(&t, thread_func, (void *)(long)depth);

  while (1) {
    gthread_yield();
    // Exit if thread done? accessing t->state is racy/hard from here without
    // helper Just run forever for demo
  }
  return 0;
}
