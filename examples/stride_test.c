#include "gthread.h"
#include <stdio.h>
#include <unistd.h>


// We can't easily modify tickets via public API yet, let's access struct hack
// or add API For now, I will add a helper in this file to modify tickets if
// headers allow, OR I should update gthread.h to expose ticket setting.

// Let's add gthread_set_tickets to gthread.c / gthread.h
// But for now, since we have the struct definition, we can cast.

void thread_func(void *arg) {
  long id = (long)arg;
  // We want to see how many times each runs
  for (int i = 0; i < 5; i++) {
    printf("Thread %ld (tickets via hack) running\n", id);
    gthread_yield();
  }
}

int main(void) {
  gthread_init();

  gthread_t *t1, *t2;
  gthread_create(&t1, thread_func, (void *)1);
  gthread_create(&t2, thread_func, (void *)2);

  // Hack: Set tickets
  // t1: 100 tickets -> Stride 100
  // t2: 50 tickets -> Stride 200
  // t1 should run twice as often as t2 ideally, but with yield() it forces
  // switch. Stride scheduling works best with preemption or CPU intensive
  // tasks. With strict yield(), it will follow pass order.

  // Let's manually set tickets (assuming we can access fields if we include
  // internal headers or just cast) Since gthread.h exposes the struct, we can
  // validly do:
  t1->tickets = 100;
  t1->stride = 10000 / 100; // 100
  t1->pass = 0;

  t2->tickets = 50;
  t2->stride = 10000 / 50; // 200
  t2->pass = 0;

  printf("Created T1 (Tickets=100) and T2 (Tickets=50)\n");

  // Run
  // T1 (0), T2 (0) -> Pick T1. T1 Pass -> 100. Stack: T2 (0), T1 (100)
  // User yields -> Pick T2. T2 Pass -> 200. Stack: T1 (100), T2 (200)
  // Pick T1. T1 Pass -> 200. Stack: T2 (200), T1 (200)
  // Pick T2 (tie break? FIFO? My heap doesn't guarantee stability but usually
  // top).

  while (1) {
    gthread_yield();
    // Break when threads done? We don't have is_done check exposed easily.
    // Validation check: Just run for a loop
    static int loops = 0;
    if (loops++ > 20)
      break;
  }

  return 0;
}
