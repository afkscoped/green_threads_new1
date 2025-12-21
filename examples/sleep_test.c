#include "dashboard.h"
#include "gthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


void sleep_task(void *arg) {
  long ms = (long)arg;
  printf("Thread %lu sleeping for %ld ms...\n", gthread_get_id(), ms);
  gthread_sleep(ms);
  printf("Thread %lu woke up!\n", gthread_get_id());
}

int main(void) {
  gthread_init();
  dashboard_start(9090);

  printf("=== Sleep/Timer Demo (Interactive) ===\n");
  printf("Open http://localhost:9090 to see thread states change (RUNNING -> "
         "SLEEPING -> RUNNABLE)\n");

  int count;
  printf("Enter number of sleep threads: ");
  scanf("%d", &count);

  for (int i = 0; i < count; i++) {
    long ms = (rand() % 5000) + 1000; // 1-6s
    gthread_t *t;
    gthread_create(&t, sleep_task, (void *)ms);
  }

  while (1) {
    gthread_yield();
  }
  return 0;
}
