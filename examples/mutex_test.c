#include "dashboard.h"
#include "gthread.h"
#include "sync.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


gmutex_t mutex;
int shared_resource = 0;

void worker(void *arg) {
  long id = (long)arg;
  for (int i = 0; i < 1000; i++) {
    gmutex_lock(&mutex);

    // Critical Section
    int temp = shared_resource;
    // Simulate work
    for (volatile int k = 0; k < 100; k++)
      ;
    shared_resource = temp + 1;

    gmutex_unlock(&mutex);

    if (i % 100 == 0)
      gthread_yield();
  }
  printf("Thread %ld done.\n", id);
}

int main(void) {
  gthread_init();
  gmutex_init(&mutex);
  dashboard_start(9090);

  printf("=== Mutex Synchronization Demo (Interactive) ===\n");
  printf("Visualizes locking/blocking. Open http://localhost:9090\n");
  printf("Look at the 'Synchronization' section on dashboard.\n");

  int num_threads;
  printf("Enter number of threads: ");
  if (scanf("%d", &num_threads) != 1)
    num_threads = 2; // Default if fail
  if (num_threads < 1)
    num_threads = 2;

  for (long i = 0; i < num_threads; i++) {
    gthread_t *t;
    gthread_create(&t, worker, (void *)(long)(i + 1));
  }

  printf("Threads launched. Check dashboard for BLOCKED (red) threads waiting "
         "for Mutex.\n");

  while (1) {
    gthread_yield();
    sleep(1);
  }
  return 0;
}
