#include "gthread.h"
#include "io.h"
#include "scheduler.h"
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 4096

const char *response_template = "HTTP/1.1 200 OK\r\n"
                                "Content-Type: text/plain\r\n"
                                "Content-Length: %d\r\n"
                                "Connection: close\r\n"
                                "\r\n"
                                "%s";

void handle_client(void *arg) {
  long client_fd = (long)arg;
  char buffer[BUFFER_SIZE] = {0};

  // Read request
  int n = gthread_read(client_fd, buffer, BUFFER_SIZE - 1);
  if (n > 0) {
    printf("Received request: %.50s...\n", buffer); // Log first 50 chars

    char body[1024];
    snprintf(body, sizeof(body), "Hello from Green Thread %ld!",
             g_current_thread ? g_current_thread->id : 0);

    char response[BUFFER_SIZE];
    int len = snprintf(response, sizeof(response), response_template,
                       strlen(body), body);

    gthread_write(client_fd, response, len);
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

  printf("HTTP Server listening on port %d\n", PORT);

  while (1) {
    int new_socket = gthread_accept(server_fd, NULL, NULL);
    if (new_socket >= 0) {
      // Spawn a new green thread for each connection
      gthread_t *t;
      gthread_create(&t, handle_client, (void *)(long)new_socket);
    }
  }
}

int main(void) {
  gthread_init();

  gthread_t *server_thread;
  gthread_create(&server_thread, server_loop, NULL);

  gthread_join(server_thread, NULL);
  return 0;
}
