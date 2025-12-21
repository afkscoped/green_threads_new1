#include "gthread.h"
#include "io.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


#define PORT 9090

void client_task(void *arg) {
  (void)arg;
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);
  inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

  // Sleep to let server start
  gthread_sleep(100);

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    printf("Client: connect failed\n");
    return;
  }

  const char *msg = "Hello from Green Thread Client!";
  gthread_write(sock, msg, strlen(msg));

  char buffer[1024] = {0};
  gthread_read(sock, buffer, 1024);
  printf("Client received: %s\n", buffer);
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
  listen(server_fd, 3);

  printf("Server listening on port %d\n", PORT);

  // Accept
  int new_socket = gthread_accept(server_fd, NULL, NULL);
  printf("Server: accepted connection\n");

  char buffer[1024] = {0};
  gthread_read(new_socket, buffer, 1024);
  printf("Server received: %s\n", buffer);

  const char *resp = "Hello from Server!";
  gthread_write(new_socket, resp, strlen(resp));

  close(new_socket);
  close(server_fd);
}

int main(void) {
  gthread_init();

  gthread_t *t1, *t2;
  gthread_create(&t1, server_task, NULL);
  gthread_create(&t2, client_task, NULL);

  gthread_join(t1, NULL);
  gthread_join(t2, NULL);

  printf("Main: IO Test Done.\n");
  return 0;
}
