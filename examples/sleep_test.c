#include "gthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>


static uint64_t get_time_ms(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

void sleep_worker(void *arg) {
  long ms = (long)arg;
  printf("Thread starting. Sleeping for %ld ms\n", ms);
  uint64_t start = get_time_ms();
  gthread_sleep(ms);
  uint64_t end = get_time_ms();
  printf("Thread woke up after %lu ms (Target: %ld)\n", end - start, ms);
}

int main(void) {
  gthread_init();

  int n;
  printf("Enter number of sleeping threads: ");
  if (scanf("%d", &n) != 1)
    n = 3;

  for (int i = 0; i < n; i++) {
    int ms;
    printf("Enter sleep time (ms) for Thread %d: ", i + 1);
    if (scanf("%d", &ms) != 1)
      ms = 1000;

    gthread_t *t;
    gthread_create(&t, sleep_worker, (void *)(long)ms);
  }

  while (1) {
    gthread_yield();
  }
  return 0;
}
