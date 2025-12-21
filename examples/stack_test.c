#include "gthread.h"
#include <stdio.h>
#include <unistd.h>


// Recursive function to eat stack
void deep_recursion(int depth) {
  char buffer[1024]; // 1KB per frame
  // Prevent optimization
  sprintf(buffer, "Depth %d", depth);

  if (depth > 0) {
    if (depth % 10 == 0) {
      printf("Stack test: Depth %d\n", depth);
      gthread_yield(); // Yield inside recursion
    }
    deep_recursion(depth - 1);
  }
}

void thread_func(void *arg) {
  long id = (long)arg;
  printf("Thread %ld starting recursion...\n", id);
  // 50KB depth approx (50 * 1KB). Default stack 64KB. Should be fine.
  deep_recursion(40);
  printf("Thread %ld finished recursion\n", id);
}

int main(void) {
  gthread_init();

  gthread_t *t;
  // Create thread
  gthread_create(&t, thread_func, (void *)1);

  printf("Created stack test thread. Running...\n");

  // Loop
  while (1) {
    gthread_yield();
    static int loops = 0;
    if (loops++ > 100)
      break;
  }

  return 0;
}
