#include "dashboard.h"
#include "gthread.h"
#include <fcntl.h> // For STDIN_FILENO
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


void echo_task(void *arg) {
  (void)arg;
  char buf[128];
  printf(
      "Echo Task: Type something and press ENTER (I am waiting on stdin...)\n");

  while (1) {
    // gthread_read on stdin (FD 0)
    // This will block this green thread until input is available
    // Dashboard will show this thread as WAITING on FD 0
    int n = gthread_read(STDIN_FILENO, buf, sizeof(buf) - 1);
    if (n > 0) {
      buf[n] = 0;
      printf("Echo: %s", buf);
    } else {
      break;
    }
  }
}

int main(void) {
  gthread_init();
  dashboard_start(9090);

  printf("=== IO Integration Demo (Interactive) ===\n");
  printf("Visualizes threads blocked on I/O. Open http://localhost:9090\n");
  printf("Check 'Waiting I/O' section.\n");

  gthread_t *t;
  gthread_create(&t, echo_task, NULL);

  // Keep main alive
  while (1) {
    gthread_yield();
  }
  return 0;
}
