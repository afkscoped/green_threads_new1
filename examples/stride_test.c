#include "gthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Struct to pass args
typedef struct {
  long id;
  int iterations;
} thread_arg_t;

void thread_func(void *arg) {
  thread_arg_t *args = (thread_arg_t *)arg;
  long id = args->id;
  int iter = args->iterations;

  // Calculate print interval (e.g., 10 times total)
  int print_interval = iter / 10;
  if (print_interval < 1)
    print_interval = 1;

  for (int i = 0; i < iter; i++) {
    if (i % print_interval == 0) {
      // Use basic printf, no complex formatting
      printf("Thread %ld running (Iter %d/%d)\n", id, i, iter);
    }
    gthread_yield();
  }
  printf("Thread %ld finished.\n", id);
  free(args); // Free the malloc'd struct
}

int main(void) {
  gthread_init();

  int num_threads;
  printf("Enter number of threads: ");
  if (scanf("%d", &num_threads) != 1 || num_threads < 1)
    num_threads = 2;

  int iterations;
  printf("Enter workload duration (iterations per thread, e.g. 500): ");
  if (scanf("%d", &iterations) != 1 || iterations < 1)
    iterations = 100;

  for (int i = 0; i < num_threads; i++) {
    int tickets;
    printf("Enter tickets for Thread %d: ", i + 1);
    if (scanf("%d", &tickets) != 1 || tickets < 1)
      tickets = 100;

    // Allocate args
    thread_arg_t *args = malloc(sizeof(thread_arg_t));
    args->id = i + 1;
    args->iterations = iterations;

    gthread_t *t;
    gthread_create(&t, thread_func, (void *)args);

    if (t) {
      t->tickets = tickets;
      t->stride = 10000 / tickets;
      printf("Created T%d with %d tickets (Stride: %lu)\n", i + 1, tickets,
             t->stride);
    }
  }

  printf("Starting scheduler... (CTRL+C to stop if infinite)\n");
  while (1) {
    gthread_yield();
    // In a real scenario we'd join, but here loop forever is fine or until
    // threads die
  }
  return 0;
}
