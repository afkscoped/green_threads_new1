#include "gthread.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>


// Helper to get time
static uint64_t get_time_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

void sleep_worker(void *arg) {
  long id = (long)arg;
  uint64_t sleep_time = id * 500; // 500, 1000, 1500 ms

  printf("Thread %ld starting. Sleeping for %lu ms\n", id, sleep_time);

  uint64_t start = get_time_ms();
  gthread_sleep(sleep_time);
  uint64_t end = get_time_ms();

  printf("Thread %ld woke up. Delta: %lu ms\n", id, end - start);
}

int main(void) {
  gthread_init();

  gthread_t *t1, *t2, *t3;
  gthread_create(&t1, sleep_worker, (void *)1);
  gthread_create(&t2, sleep_worker, (void *)2);
  gthread_create(&t3, sleep_worker, (void *)3);

  printf("Main: created 3 threads.\n");

  gthread_join(t1, NULL);
  gthread_join(t2, NULL);
  gthread_join(t3, NULL);

  printf("Main: All done.\n");
  return 0;
}
