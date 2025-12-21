#include "gthread.h"
#include <stdio.h>
#include <stdlib.h>

void print_menu(void) {
  printf("\n=== Green Threads Demo Runner ===\n");
  printf("1. Basic Threads\n");
  printf("2. Stride Scheduling (Interactive + Dashboard)\n");
  printf("3. Stack Management (Interactive + Dashboard)\n");
  printf("4. Synchronization (Mutex/Cond)\n");
  printf("5. Sleep/Timers (Interactive + Dashboard)\n");
  printf("6. IO Integration (Interactive + Dashboard)\n");
  printf("7. HTTP Server (Port 8080)\n");
  printf("8. Parallel Matrix Multiplication\n");
  printf("9. Web Dashboard (Port 8080)\n");
  printf("10. Advanced Dashboard (Standalone :9090)\n");
  printf("0. Exit\n");
  printf("Select demo: ");
}

int main(void) {
  int choice;
  while (1) {
    print_menu();
    if (scanf("%d", &choice) != 1)
      break;

    switch (choice) {
    case 1:
      system("./build/basic_threads");
      break;
    case 2:
      system("./build/stride_test");
      break;
    case 3:
      system("./build/stack_test");
      break;
    case 4:
      system("./build/mutex_test");
      break;
    case 5:
      system("./build/sleep_test");
      break;
    case 6:
      system("./build/io_test");
      break;
    case 7:
      system("./build/http_server");
      break;
    case 8:
      system("./build/matrix_mul");
      break;
    case 9:
      printf("Starting Web Dashboard... Press Ctrl+C to stop.\n");
      system("./build/web_dashboard");
      break;
    case 10:
      printf("Starting Advanced Dashboard... Press Ctrl+C to stop.\n");
      system("./build/advanced_dashboard");
      break;
    case 0:
      exit(0);
    default:
      printf("Invalid choice\n");
    }
  }
  return 0;
}
