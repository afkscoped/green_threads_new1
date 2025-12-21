#include "gthread.h"
#include <stdio.h>
#include <unistd.h>


void thread_func(void *arg) {
  long id = (long)arg;
  for (int i = 0; i < 3; i++) {
    printf("Thread %ld is running (iteration %d)\n", id, i);
    gthread_yield();
  }
  printf("Thread %ld finished\n", id);
}

int main(void) {
  printf("Main: Starting basic threads test\n");
  gthread_init();

  gthread_t *threads[5];
  for (long i = 1; i <= 5; i++) {
    if (gthread_create(&threads[i - 1], thread_func, (void *)i) != 0) {
      fprintf(stderr, "Failed to create thread %ld\n", i);
      return 1;
    }
  }

  printf("Main: Created 5 threads. Yielding to scheduler...\n");

  // Simple loop to keep yielding until (hopefully) all are done
  // Real implementation would use gthread_join() or check status
  for (int i = 0; i < 20; i++) {
    gthread_yield();
  }

  printf("Main: Exiting\n");
  return 0;
}
