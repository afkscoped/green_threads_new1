#include "gthread.h"
#include "io.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define PORT 9091

// Shared socket for input listener to write to
volatile int g_client_socket = -1;

void client_receiver_task(void *arg) {
  (void)arg;
  char buffer[128];
  while (1) {
    if (g_client_socket < 0) {
      gthread_sleep(100);
      continue;
    }

    int n = gthread_read(g_client_socket, buffer, sizeof(buffer) - 1);
    if (n > 0) {
      buffer[n] = '\0';
      printf("\rServer Echo: %s\n> ",
             buffer); // \r to clear input prompt overlap
      fflush(stdout);
    } else if (n == 0) {
      printf("\nServer closed connection.\n");
      close(g_client_socket);
      g_client_socket = -1;
      break;
    } else {
      // Yield on error/block
      gthread_yield();
    }
  }
}

void client_connect_task(void *arg) {
  (void)arg;
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);
  inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

  // Simple retry loop
  while (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    gthread_sleep(500);
  }

  printf("Connected to server on port %d!\n> ", PORT);
  fflush(stdout);

  g_client_socket = sock;

  // Spawn receiver for this socket
  gthread_t *recv;
  gthread_create(&recv, client_receiver_task, NULL);

  // Monitor indefinitely (or could exit thread and let global persist)
  while (g_client_socket >= 0) {
    gthread_sleep(1000);
  }
}

// Handles a single server connection continuously
void connection_handler(void *arg) {
  int sock = (long)arg;
  char buffer[128];
  while (1) {
    int n = gthread_read(sock, buffer, sizeof(buffer));
    if (n > 0) {
      gthread_write(sock, buffer, n); // Echo back
    } else if (n == 0) {
      break; // Disconnect
    }
    gthread_yield();
  }
  close(sock);
}

void server_task(void *arg) {
  (void)arg;
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in address;
  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  bind(server_fd, (struct sockaddr *)&address, sizeof(address));
  listen(server_fd, 100);

  int flags = fcntl(server_fd, F_GETFL, 0);
  fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

  printf("Chat Server listening on %d\n", PORT);

  while (1) {
    int new_socket = gthread_accept(server_fd, NULL, NULL);
    if (new_socket >= 0) {
      // Spawn a handler for this client
      gthread_t *handler;
      gthread_create(&handler, connection_handler, (void *)(long)new_socket);
    }
    gthread_yield();
  }
}

void input_listener(void *arg) {
  (void)arg;
  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
  char buffer[1024];

  while (1) {
    gthread_wait_io(STDIN_FILENO, POLLIN);
    int n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
    if (n > 0) {
      buffer[n] = '\0';
      // Trim newline
      if (buffer[n - 1] == '\n')
        buffer[n - 1] = '\0';

      if (g_client_socket >= 0) {
        gthread_write(g_client_socket, buffer, strlen(buffer));
        // Local echo handled by receiver? No, receiver prints server echo.
        // printf("Sent.\n> ");
        // fflush(stdout);
      } else {
        printf("Not connected.\n> ");
        fflush(stdout);
      }
    }
  }
}

int main(void) {
  gthread_init();

  gthread_t *server;
  gthread_create(&server, server_task, NULL);

  gthread_sleep(100);

  // Interactive client setup
  gthread_t *client_setup;
  gthread_create(&client_setup, client_connect_task, NULL);

  // Input listener (Writer)
  gthread_t *input;
  gthread_create(&input, input_listener, NULL);

  while (1) {
    gthread_yield();
    gthread_sleep(100);
  }
  return 0;
}
