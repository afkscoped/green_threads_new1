#include "dashboard.h"
#include "gthread.h"
#include "runtime_stats.h"
#include <stdio.h>
#include <unistd.h>


/* This is the standalone runner for the advanced dashboard. */

void dummy_load(void *arg) {
  long id = (long)arg;
  while (1) {
    gthread_yield();
    // spin a bit to show usage
    for (int i = 0; i < 1000; i++)
      asm("nop");
  }
}

int main(void) {
  gthread_init();

  printf("Starting Standalone Advanced Dashboard on :9090\n");
  dashboard_start(9090);

  // Spawn some dummy threads so the user sees something
  gthread_t *t;
  gthread_create(&t, dummy_load, (void *)1);
  gthread_create(&t, dummy_load, (void *)2);
  runtime_set_tickets(t->id, 50);

  printf("Dashboard running. Press Ctrl+C to exit.\n");

  while (1) {
    gthread_yield();
    sleep(1);
  }
  return 0;
}
