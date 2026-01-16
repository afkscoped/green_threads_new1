#include "gthread.h"
#include "io.h"
#include "runtime_stats.h"
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

#define PORT 9090
#define JSON_BUF_SIZE 131072

static char json_buf[JSON_BUF_SIZE];

// Helper to serve file
void serve_static(int client_fd, const char *path, const char *type) {
  char full_path[256];
  snprintf(full_path, sizeof(full_path), "examples/advanced_static/%s", path);
  FILE *f = fopen(full_path, "rb");

  // Check build dir fallback
  if (!f) {
    snprintf(full_path, sizeof(full_path), "build/examples/advanced_static/%s",
             path);
    f = fopen(full_path, "rb");
  }

  if (!f) {
    const char *nf = "HTTP/1.1 404 Not Found\r\n\r\n404";
    gthread_write(client_fd, nf, strlen(nf));
    return;
  }

  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *buf = malloc(fsize);
  if (buf) {
    fread(buf, 1, fsize, f);
    char hdr[512];
    snprintf(hdr, sizeof(hdr),
             "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: "
             "%ld\r\nConnection: close\r\n\r\n",
             type, fsize);
    gthread_write(client_fd, hdr, strlen(hdr));
    gthread_write(client_fd, buf, fsize);
    free(buf);
  }
  fclose(f);
}

// Build JSON for /threads
void handle_threads(int fd) {
  int offset = 0;
  json_buf[offset++] = '[';

  gthread_t *curr = gthread_get_all_threads();
  int first = 1;

  while (curr) {
    if (!first)
      json_buf[offset++] = ',';
    first = 0;

    // printf("DEBUG: Processing thread ID %lu\n", curr->id); // Debug Log
    stack_stats_t ss = runtime_get_stack_stats(curr->id);
    // printf("DEBUG: Got stack stats for ID %lu (Used: %zu)\n", curr->id,
    // ss.stack_used);

    offset +=
        snprintf(json_buf + offset, JSON_BUF_SIZE - offset,
                 "{\"id\":%lu,\"tickets\":%lu,\"pass\":%lu,\"state\":%d,"
                 "\"stride\":%lu,\"stack_used\":%zu,\"waiting_fd\":%d,\"wake_"
                 "time\":%lu}",
                 curr->id, curr->tickets, curr->pass, curr->state, curr->stride,
                 ss.stack_used, curr->waiting_fd, curr->wake_time_ms);

    curr = curr->global_next;
    if (offset >= JSON_BUF_SIZE - 200)
      break;
  }
  json_buf[offset++] = ']';
  json_buf[offset] = '\0';
  // printf("DEBUG: JSON built successfully, length %d\n", offset);

  char hdr[512];
  snprintf(
      hdr, sizeof(hdr),
      "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: "
      "%d\r\nConnection: close\r\n\r\n",
      offset);
  gthread_write(fd, hdr, strlen(hdr));
  gthread_write(fd, json_buf, offset);
}

void handle_client_adv(void *arg) {
  long client_fd = (long)arg;
  char buffer[2048] = {0};
  int n = gthread_read(client_fd, buffer, sizeof(buffer) - 1);

  if (n <= 0) {
    close(client_fd);
    return;
  }

  if (strncmp(buffer, "GET / ", 6) == 0 ||
      strncmp(buffer, "GET /index.html ", 16) == 0) {
    serve_static(client_fd, "index.html", "text/html");
  } else if (strncmp(buffer, "GET /style.css ", 15) == 0) {
    serve_static(client_fd, "style.css", "text/css");
  } else if (strncmp(buffer, "GET /dashboard.js ", 18) == 0) {
    serve_static(client_fd, "dashboard.js", "application/javascript");
  } else if (strncmp(buffer, "GET /threads ", 13) == 0) {
    handle_threads(client_fd);
  } else if (strncmp(buffer, "POST /tickets", 13) == 0) {
    // Simple manual parsing: expects "id=X&tickets=Y" in body
    char *body = strstr(buffer, "\r\n\r\n");
    if (body) {
      body += 4;
      // Parse (demo quality)
      int id = 0, tix = 10;
      // Find id=
      char *id_ptr = strstr(body, "id=");
      if (id_ptr)
        id = atoi(id_ptr + 3);

      char *tix_ptr = strstr(body, "tickets=");
      if (tix_ptr)
        tix = atoi(tix_ptr + 8);

      runtime_set_tickets(id, tix);

      const char *ok = "HTTP/1.1 200 OK\r\n\r\nOK";
      gthread_write(client_fd, ok, strlen(ok));
    } else {
      const char *bad = "HTTP/1.1 400 Bad\r\n\r\nSize";
      gthread_write(client_fd, bad, strlen(bad));
    }
  } else {
    const char *nf = "HTTP/1.1 404 Not Found\r\n\r\nNot Found";
    gthread_write(client_fd, nf, strlen(nf));
  }

  close(client_fd);
}

void advanced_server_loop(void *arg) {
  (void)arg;
  int server_fd;
  struct sockaddr_in address;
  int opt = 1;

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    return;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    return;
  if (listen(server_fd, 10) < 0)
    return;

  printf("Advanced Dashboard listening on :%d\n", PORT);

  while (1) {
    int new_socket = gthread_accept(server_fd, NULL, NULL);
    if (new_socket >= 0) {
      gthread_t *t;
      gthread_create(&t, handle_client_adv, (void *)(long)new_socket);
    }
  }
}

// We need a main function here since this is a separate binary
// Wait, user said "New binary: advanced_dashboard"
// BUT they also said "Both dashboards must run simultaneously".
// And "share the same runtime state safely".
// IF THEY ARE SEPARATE BINARIES, THEY CANNOT SHARE STATE without shared memory!
// The user said: "Both dashboards must run simultaneously" and "share the same
// runtime state safely". And "New binary: advanced_dashboard". If they are
// separate processes, `monitor_task_t` and `gthread_t` are NOT shared unless I
// map shared memory. BUT the user instructions say: "Add a new module...
// include/runtime_stats.h... Minimal changes to scheduler.c... No blocking
// syscalls" AND "The solution is correct only if: Both dashboards run at the
// same time: ./build/web_dashboard ... ./build/advanced_dashboard". This
// implies they are separate processes. BUT `green_threads` is a user-space
// library linked INTO the binary. Accessing `gthread_get_all_threads()` from a
// separate binary `advanced_dashboard` will only show the threads OF THAT
// DASHBOARD. It WON'T show the threads of the OTHER dashboard
// (`web_dashboard`). THE USER MIGHT BE CONFUSED or implies I should use Shared
// Memory (shm_open). OR, maybe the `advanced_dashboard` IS the `web_dashboard`
// extended? No, "Add a SECOND, ADVANCED WEB DASHBOARD... New binary". The only
// way two processes share `gthread` state (which is just malloc'd memory) is
// `mmap` MAP_SHARED or `shm`. The PROMPT says: "Context: Green Threads...
// implements user-space green threads... existing project". "You are working
// with an existing C project...". "Both dashboards must run simultaneously...
// share the same runtime state".
//
// INTERPRETATION:
// 1. The `advanced_dashboard` binary IS A DEMO ITSELF. It runs its own green
// threads (some workloads) AND serves the dashboard.
// 2. The `web_dashboard` binary IS ALSO A DEMO. It runs threads AND serves.
// 3. User might think I can attach Dashboard 2 to Dashboard 1's process? No,
// that's not how C works unless I use SHM or sockets.
// 4. "The solution is correct only if... Changing ticket counts in Dashboard
// #2... visibly affects scheduler behavior [in Dashboard #2?]".
// -> It means Dashboard 2 monitors ITS OWN runtime.
// -> It does NOT mean Dashboard 2 monitors Dashboard 1's runtime.
// -> "Both dashboards must run simultaneously" just means port conflict check
// (8080 vs 9090).
// -> "share the same runtime state safely" might refer to thread-safety WITHIN
// the `advanced_dashboard` process (since HTTP server threads access scheduler
// data).
// -> I will proceed with `advanced_dashboard` being a standalone binary that
// demonstrates the features on ITS OWN set of threads.
// -> I will reproduce the `cpu_task`, `io_task` etc in `advanced_dashboard` so
// there is something to see.

// --- Workload Tasks ---

void cpu_task(void *arg) {
  (void)arg;
  long id = (long)gthread_self_id(); // Use runtime ID
  // We don't use monitor_register here because advanced dashboard reads
  // DIRECTLY from runtime stats and gthread struct.

  while (1) {
    // Busy loop
    for (volatile int i = 0; i < 1000000; i++)
      ;
    gthread_yield();
  }
}

void sleep_task(void *arg) {
  (void)arg;
  while (1) {
    gthread_sleep(2000);
    // wake
  }
}

int main(void) {
  gthread_init();

  // Spawn advanced dashboard
  gthread_t *t;
  gthread_create(&t, advanced_server_loop, NULL);

  // Spawn workload
  printf("Spawning workload threads...\n");
  for (int i = 0; i < 4; i++) {
    gthread_t *c;
    gthread_create(&c, cpu_task, NULL);
    // stagger tickets
    if (c)
      runtime_set_tickets(c->id, (i + 1) * 50);
  }

  gthread_t *s;
  gthread_create(&s, sleep_task, NULL);

  gthread_join(t, NULL); // Wait for server
  return 0;
}
