#include "../src/scheduler.c"
#include <assert.h>
#include <stdio.h>

// We include the C file directly to test static functions or internals if
// needed, or just header if strictly public API. For simple testing, direct is
// fine.

/* Mock Green Thread */
typedef struct {
  int id;
} mock_thread_t;

void test_rr_scheduler() {
  printf("Testing Round Robin Scheduler...\n");

  green_thread_t t1 = {.id = 1};
  green_thread_t t2 = {.id = 2};
  green_thread_t t3 = {.id = 3};

  scheduler_enqueue(&t1);
  scheduler_enqueue(&t2);
  scheduler_enqueue(&t3);

  green_thread_t *n = scheduler_next();
  assert(n->id == 1);

  n = scheduler_next();
  assert(n->id == 2);

  n = scheduler_next();
  assert(n->id == 3);

  n = scheduler_next();
  assert(n == NULL);

  printf("Round Robin Scheduler Test Passed!\n");
}

int main() {
  test_rr_scheduler();
  return 0;
}
