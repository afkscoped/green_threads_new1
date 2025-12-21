#include "dashboard.h"
#include "gthread.h"
#include "io.h"
#include "runtime_stats.h"
#include "scheduler.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h> // Ensure socket headers are present
#include <sys/types.h>
#include <unistd.h>

#define JSON_BUF_SIZE 131072

// Static buffer to avoid large stack allocation
static char json_buf[JSON_BUF_SIZE];

// Serve static files
static void serve_static(int client_fd, const char *path, const char *type) {
  char full_path[256];
  // try local examples/advanced_static first
  snprintf(full_path, sizeof(full_path), "examples/advanced_static/%s", path);
  FILE *f = fopen(full_path, "rb");

  if (!f) {
    // try build fallback
    snprintf(full_path, sizeof(full_path), "build/examples/advanced_static/%s",
             path);
    f = fopen(full_path, "rb");
  }

  if (!f) {
    // Try one more relative path (project root from build)
    snprintf(full_path, sizeof(full_path), "../examples/advanced_static/%s",
             path);
    f = fopen(full_path, "rb");
  }

  if (!f) {
    const char *nf =
        "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n404 Not Found";
    gthread_write(client_fd, nf, strlen(nf));
    return;
  }

  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  char *buf = malloc(fsize + 1); // +1 safety
  if (buf) {
    if (fread(buf, 1, fsize, f) == (size_t)fsize) {
      char hdr[512];
      snprintf(hdr, sizeof(hdr),
               "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: "
               "%ld\r\nConnection: close\r\n\r\n",
               type, fsize);
      gthread_write(client_fd, hdr, strlen(hdr));
      gthread_write(client_fd, buf, fsize);
    }
    free(buf);
  }
  fclose(f);
}

// JSON Handler
static void handle_threads(int fd) {
  int offset = 0;
  json_buf[offset++] = '[';

  gthread_t *curr = gthread_get_all_threads();
  int first = 1;
  int items = 0;

  while (curr && items < 500) { // Limit items to prevent overflow/starvation
    if (!first) {
      if (offset >= JSON_BUF_SIZE - 200)
        break;
      json_buf[offset++] = ',';
    }
    first = 0;

    stack_stats_t ss = runtime_get_stack_stats(curr->id);

    offset += snprintf(json_buf + offset, JSON_BUF_SIZE - offset,
                       "{\"id\":%lu,\"tickets\":%d,\"pass\":%lu,\"state\":%d,"
                       "\"stride\":%lu,\"stack_used\":%zu,\"waiting_fd\":%d,"
                       "\"wake_time\":%lu}",
                       (unsigned long)curr->id, curr->tickets,
                       (unsigned long)curr->pass, curr->state,
                       (unsigned long)curr->stride, ss.stack_used,
                       curr->waiting_fd, (unsigned long)curr->wake_time_ms);

    curr = curr->global_next;
    items++;

    if (offset >= JSON_BUF_SIZE - 200)
      break;
  }
  json_buf[offset++] = ']';
  json_buf[offset] = '\0';

  char hdr[512];
  snprintf(
      hdr, sizeof(hdr),
      "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: "
      "%d\r\nConnection: close\r\n\r\n",
      offset);
  gthread_write(fd, hdr, strlen(hdr));
  gthread_write(fd, json_buf, offset);
}

static void handle_client(void *arg) {
  long client_fd = (long)arg;
  char buffer[2048] = {0};

  // Read request
  int n = gthread_read(client_fd, buffer, sizeof(buffer) - 1);
  if (n <= 0) {
    close(client_fd);
    return;
  }

  // Router
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
    // Parse ID and Tickets
    char *body = strstr(buffer, "\r\n\r\n");
    if (body) {
      body += 4;
      int id = 0, tix = 10;
      char *id_p = strstr(body, "id=");
      if (id_p)
        id = atoi(id_p + 3);
      char *tix_p = strstr(body, "tickets=");
      if (tix_p)
        tix = atoi(tix_p + 8);

      runtime_set_tickets(id, tix);
      const char *msg = "HTTP/1.1 200 OK\r\nConnection: close\r\n\r\nOK";
      gthread_write(client_fd, msg, strlen(msg));
    } else {
      const char *msg =
          "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\nEmpty";
      gthread_write(client_fd, msg, strlen(msg));
    }
  } else {
    const char *nf = "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n404";
    gthread_write(client_fd, nf, strlen(nf));
  }

  close(client_fd);
}

static void dashboard_server_task(void *arg) {
  int port = (int)(long)arg;
  int server_fd;
  struct sockaddr_in address;
  int opt = 1;

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
    return;
  }

  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    perror("setsockopt");
    return;
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    return;
  }

  if (listen(server_fd, 10) < 0) {
    perror("listen");
    return;
  }

  printf("[Dashboard] Listening on http://localhost:%d\n", port);

  // Set non-blocking
  int flags = fcntl(server_fd, F_GETFL, 0);
  fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

  while (1) {
    // Wait for connection
    scheduler_register_io_wait(server_fd, POLLIN);

    int client_sock = accept(server_fd, NULL, NULL);
    if (client_sock >= 0) {
      gthread_t *t;
      gthread_create(&t, handle_client, (void *)(long)client_sock);
    }
  }
}

void dashboard_start(int port) {
  gthread_t *t;
  gthread_create(&t, dashboard_server_task, (void *)(long)port);
}
