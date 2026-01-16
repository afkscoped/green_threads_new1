#define _GNU_SOURCE
#include "dual_run.h"
#include "green_thread.h"
#include "internal.h"
#include "metrics.h"
#include "workloads.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct {
  int n_green;
  int n_pthread;
} spawn_params_t;

void *spawn_helper(void *v) {
  spawn_params_t *args = (spawn_params_t *)v;
  // Use the counts passed in
  extern void matrix_mul_task(void *arg);
  // Pass a dummy 'iterations' count for the matrix task logic?
  // matrix_mul_task expects 'int *' as arg.
  // We need to pass that too. Let's assume fixed iteration count for now or
  // pass it.

  int *iter_arg = malloc(sizeof(int));
  *iter_arg =
      -1; // Infinite iterations, relies on Stop (spawn:matrix:0) to terminate

  g_any_workload_running = 1; // Mark as active
  dual_run_workload(matrix_mul_task, iter_arg, args->n_green, args->n_pthread);
  g_any_workload_running = 0; // Mark as finished

  free(args);
  return NULL;
}

// ... inside tcp_server_thread loop ...

static int server_fd = -1;
static int client_fd = -1;
static pthread_mutex_t client_lock = PTHREAD_MUTEX_INITIALIZER;

void *broadcast_thread(void *arg) {
  (void)arg;
  char buffer[4096];

  while (1) {
    usleep(100000); // 100ms

    // Run Anomaly Detection
    extern void aura_tick(void);
    aura_tick();

    pthread_mutex_lock(&client_lock);
    if (client_fd != -1) {
      memset(buffer, 0, 4096);
      metrics_to_json(buffer, 4096);
      strcat(buffer, "\n"); // Delimiter

      if (send(client_fd, buffer, strlen(buffer), MSG_NOSIGNAL) < 0) {
        close(client_fd);
        client_fd = -1;
      }
    }
    pthread_mutex_unlock(&client_lock);
  }
  return NULL;
}

void *tcp_server_thread(void *arg) {
  (void)arg;
  int port = 8081;
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    return NULL;

  // SOL_SOCKET and options might need specific headers or macros depending on
  // platform, but assuming standard POSIX for now.
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    return NULL;
  if (listen(server_fd, 3) < 0)
    return NULL;

  printf("[Control] Listening on port %d\n", port);

  // Launch broadcaster
  pthread_t bt;
  extern void *broadcast_thread(void *arg);
  pthread_create(&bt, NULL, broadcast_thread, NULL);
  pthread_detach(bt);

  while (1) {
    int new_socket;
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                             (socklen_t *)&addrlen)) < 0)
      continue;

    printf("[Control] Controller connected\n");

    pthread_mutex_lock(&client_lock);
    if (client_fd != -1)
      close(client_fd);
    client_fd = new_socket;
    pthread_mutex_unlock(&client_lock);

    // Read loop (Control commands)
    char buffer[1024] = {0};
    while (1) {
      int valread = read(client_fd, buffer, 1024);
      if (valread <= 0) {
        pthread_mutex_lock(&client_lock);
        close(client_fd);
        client_fd = -1;
        pthread_mutex_unlock(&client_lock);
        break;
      }

      // Command Dispatch
      // Format: spawn:matrix:count OR set:slice:ms OR set:tickets:val
      // printf("[Control] Received: %s\n", buffer); // Reduced verbosity to
      // prevent log flooding

      if (strncmp(buffer, "set:slice:", 10) == 0) {
        int val = atoi(buffer + 10);
        gt_set_global_timeslice_ms(val);
        printf("[Control] Set timeslice to %d ms\n", val);
      } else if (strncmp(buffer, "set:tickets:", 12) == 0) {
        int val = atoi(buffer + 12);
        // Ensure Stride Scheduler is active when we set tickets
        gt_set_scheduler_type(1);
        gt_set_stride_tickets(0, val);
        printf("[Control] Set tickets to %d (Stride Mode)\n", val);
      } else if (strncmp(buffer, "spawn:", 6) == 0) {
        // 1. Trigger Shutdown of existing workload
        // 1. Simple Shutdown Trigger (Fire and Forget)
        g_shutdown_workload = 1;
        usleep(200000); // Wait 200ms suitable for most stops
        g_shutdown_workload = 0;

        spawn_params_t *args = malloc(sizeof(spawn_params_t));
        args->n_green = 0;
        args->n_pthread = 0;

        if (strncmp(buffer, "spawn:matrix:", 13) == 0) {
          int count = atoi(buffer + 13);
          if (count == 0) {
            free(args);
            goto skip_spawn;
          }
          if (count <= 0)
            count = 1;
          args->n_green = count;
          args->n_pthread = count;
        } else if (strncmp(buffer, "spawn:green:", 12) == 0) {
          int count = atoi(buffer + 12);
          if (count <= 0)
            count = 1;
          args->n_green = count;
        } else if (strncmp(buffer, "spawn:pthread:", 14) == 0) {
          int count = atoi(buffer + 14);
          if (count <= 0)
            count = 1;
          args->n_pthread = count;
        } else {
          free(args);
          goto skip_spawn;
        }

        pthread_t t;
        pthread_create(&t, NULL, spawn_helper, args);
        pthread_detach(t);

      skip_spawn:;
      }
      memset(buffer, 0, 1024);
    } // end read loop (nested)
  } // end accept loop
  return NULL;
}

void *http_server_thread(void *arg) {
  (void)arg;
  int port = 8080;
  int server_fd, new_socket;
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    return NULL;

  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    perror("setsockopt"); // Optional error logging
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    return NULL;
  if (listen(server_fd, 3) < 0)
    return NULL;

  printf("[Metrics] HTTP Server listening on port %d\n", port);

  while (1) {
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                             (socklen_t *)&addrlen)) < 0) {
      continue;
    }

    char buffer[1024] = {0};
    int valread = read(new_socket, buffer, 1024);
    if (valread > 0) {
      // Simple request check
      if (strstr(buffer, "GET /metrics") != NULL) {
        char metrics_buf[4096];
        metrics_get_prometheus(metrics_buf, 4096);

        char response[8192];
        snprintf(response, sizeof(response),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/plain; version=0.0.4\r\n"
                 "\r\n"
                 "%s",
                 metrics_buf);
        write(new_socket, response, strlen(response));
      } else {
        char *resp = "HTTP/1.1 404 Not Found\r\n\r\n";
        write(new_socket, resp, strlen(resp));
      }
    }
    close(new_socket);
  }
  return NULL;
}

void start_http_server(int port) {
  (void)port;
  // 1. Control Server (TCP 8081)
  pthread_t tid_ctrl;
  pthread_create(&tid_ctrl, NULL, tcp_server_thread, NULL);
  pthread_detach(tid_ctrl);

  // 2. Metrics Server (HTTP 8080)
  pthread_t tid_http;
  pthread_create(&tid_http, NULL, http_server_thread, NULL);
  pthread_detach(tid_http);
}
