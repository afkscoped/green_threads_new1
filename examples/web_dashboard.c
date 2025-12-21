#include "gthread.h"
#include "io.h"
#include "monitor.h"
#include "scheduler.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8080
#define JSON_BUF_SIZE 131072

// --- Demo Tasks ---

// CPU Task: Updates progress 0-100
void cpu_task(void *arg) {
  (void)arg;
  int id = monitor_register("CPU");
  if (id < 0)
    return;

  for (int i = 0; i <= 100; i++) {
    monitor_update_progress(id, i);
    monitor_update_state(id, TASK_RUNNABLE);

    // Busy work
    for (volatile int k = 0; k < 500000; k++)
      ;

    gthread_yield();
  }
  monitor_mark_done(id);
}

// Sleep Task: Sleeps for random duration
void sleep_task(void *arg) {
  (void)arg;
  int id = monitor_register("TIMER");

  // 5 to 15 seconds sleep
  int ms = 5000 + (rand() % 10000);

  monitor_update_state(id, TASK_SLEEPING);
  gthread_sleep(ms);

  monitor_mark_done(id);
}

// IO Task: Simulates Waiting
void io_task(void *arg) {
  (void)arg;
  int id = monitor_register("IO");

  // Simulate waiting on FD 99
  monitor_set_waitfd(id, 99);
  monitor_update_state(id, TASK_WAITING);

  gthread_sleep(8000);

  monitor_set_waitfd(id, -1);
  monitor_mark_done(id);
}

// --- HTTP Server ---

// static char json_buf[JSON_BUF_SIZE]; // Removed to prevent race condition

// Helper to serve file
void serve_file(int client_fd, const char *path, const char *content_type) {
  // Try opening local or build path
  FILE *f = fopen(path, "rb");
  if (!f) {
    char alt_path[256];
    snprintf(alt_path, sizeof(alt_path), "examples/web_static/%s", path);
    f = fopen(alt_path, "rb");
  }

  if (!f) {
    const char *not_found = "HTTP/1.1 404 Not Found\r\n\r\n404 Not Found";
    gthread_write(client_fd, not_found, strlen(not_found));
    return;
  }

  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *buf = malloc(fsize);
  if (buf) {
    fread(buf, 1, fsize, f);

    char header[512];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: "
             "%ld\r\nConnection: close\r\n\r\n",
             content_type, fsize);

    gthread_write(client_fd, header, strlen(header));
    gthread_write(client_fd, buf, fsize);
    free(buf);
  }
  fclose(f);
}

void handle_client(void *arg) {
  long client_fd = (long)arg;
  char buffer[2048] = {0};

  int n = gthread_read(client_fd, buffer, sizeof(buffer) - 1);
  if (n <= 0) {
    close(client_fd);
    return;
  }

  // Simple routing
  if (strncmp(buffer, "GET /status ", 12) == 0) {
    char *json_buf = malloc(JSON_BUF_SIZE);
    if (!json_buf) {
      close(client_fd);
      return;
    }
    int len = monitor_build_json(json_buf, JSON_BUF_SIZE);
    if (len < 0)
      len = 0;

    char header[512];
    snprintf(
        header, sizeof(header),
        "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: "
        "%d\r\nConnection: close\r\n\r\n",
        len);

    gthread_write(client_fd, header, strlen(header));
    gthread_write(client_fd, json_buf, len);
    free(json_buf);

  } else if (strncmp(buffer, "GET /style.css ", 15) == 0) {
    serve_file(client_fd, "style.css", "text/css");
  } else if (strncmp(buffer, "GET /dashboard.js ", 18) == 0) {
    serve_file(client_fd, "dashboard.js", "application/javascript");
  } else if (strncmp(buffer, "GET / ", 6) == 0 ||
             strncmp(buffer, "GET /index.html ", 16) == 0) {
    serve_file(client_fd, "index.html", "text/html");
  } else if (strncmp(buffer, "POST /spawn?type=", 17) == 0) {
    char *type = strstr(buffer, "type=") + 5;
    gthread_t *t;
    if (strncmp(type, "cpu", 3) == 0)
      gthread_create(&t, cpu_task, NULL);
    else if (strncmp(type, "sleep", 5) == 0)
      gthread_create(&t, sleep_task, NULL);
    else if (strncmp(type, "io", 2) == 0)
      gthread_create(&t, io_task, NULL);

    const char *ok = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK";
    gthread_write(client_fd, ok, strlen(ok));
  } else {
    const char *nf = "HTTP/1.1 404 Not Found\r\n\r\nNot Found";
    gthread_write(client_fd, nf, strlen(nf));
  }

  close(client_fd);
}

void server_loop(void *arg) {
  (void)arg;
  int server_fd;
  struct sockaddr_in address;
  int opt = 1;

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
    return;
  }

  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    return;
  }

  if (listen(server_fd, 10) < 0) {
    perror("listen failed");
    return;
  }

  printf("GreenThreads Dashboard listening on :%d\n", PORT);

  while (1) {
    int new_socket = gthread_accept(server_fd, NULL, NULL);
    if (new_socket >= 0) {
      gthread_t *t;
      gthread_create(&t, handle_client, (void *)(long)new_socket);
    }
  }
}

int main(void) {
  gthread_init();
  gthread_t *t;
  // Initial demos
  for (int i = 0; i < 3; i++)
    gthread_create(&t, cpu_task, NULL);
  gthread_create(&t, sleep_task, NULL);
  gthread_create(&t, io_task, NULL);

  gthread_create(&t, server_loop, NULL);
  gthread_join(t, NULL);
  return 0;
}
