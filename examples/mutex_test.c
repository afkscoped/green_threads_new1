#include "gthread.h"
#include "sync.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFER_SIZE 5
#define ITEMS_TO_PRODUCE 20

int buffer[BUFFER_SIZE];
int count = 0;
int in = 0;
int out = 0;

gmutex_t mutex;
gcond_t cond_full;
gcond_t cond_empty;

void producer(void *arg) {
  long id = (long)arg;
  for (int i = 0; i < ITEMS_TO_PRODUCE; i++) {
    gmutex_lock(&mutex);
    while (count == BUFFER_SIZE) {
      gcond_wait(&cond_full, &mutex);
    }

    buffer[in] = i;
    in = (in + 1) % BUFFER_SIZE;
    count++;
    printf("Producer %ld produced %d (Count: %d)\n", id, i, count);

    gcond_signal(&cond_empty);
    gmutex_unlock(&mutex);

    if (i % 3 == 0)
      gthread_yield();
  }
}

void consumer(void *arg) {
  long id = (long)arg;
  for (int i = 0; i < ITEMS_TO_PRODUCE; i++) {
    gmutex_lock(&mutex);
    while (count == 0) {
      gcond_wait(&cond_empty, &mutex);
    }

    int item = buffer[out];
    out = (out + 1) % BUFFER_SIZE;
    count--;
    printf("Consumer %ld consumed %d (Count: %d)\n", id, item, count);

    gcond_signal(&cond_full);
    gmutex_unlock(&mutex);

    if (i % 4 == 0)
      gthread_yield();
  }
}

int main(void) {
  gthread_init();
  gmutex_init(&mutex);
  gcond_init(&cond_full);
  gcond_init(&cond_empty);

  int n_prod, n_cons;
  printf("Enter number of Producers: ");
  if (scanf("%d", &n_prod) != 1)
    n_prod = 1;
  printf("Enter number of Consumers: ");
  if (scanf("%d", &n_cons) != 1)
    n_cons = 1;

  for (int i = 0; i < n_prod; i++) {
    gthread_t *p;
    gthread_create(&p, producer, (void *)(long)(i + 1));
  }
  for (int i = 0; i < n_cons; i++) {
    gthread_t *c;
    gthread_create(&c, consumer, (void *)(long)(i + 1));
  }

  while (1) {
    gthread_yield();
  }
  return 0;
}
