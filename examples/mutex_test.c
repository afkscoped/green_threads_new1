#include "gthread.h"
#include "sync.h"
#include <stdio.h>
#include <unistd.h>


#define BUFFER_SIZE 5
#define ITEMS_TO_PRODUCE 20

int buffer[BUFFER_SIZE];
int count = 0;
int in = 0;
int out = 0;

gmutex_t mutex;
gcond_t cond_full;  // Wait if full
gcond_t cond_empty; // Wait if empty

void producer(void *arg) {
  long id = (long)arg;
  for (int i = 0; i < ITEMS_TO_PRODUCE; i++) {
    gmutex_lock(&mutex);
    while (count == BUFFER_SIZE) {
      // Buffer full, wait
      gcond_wait(&cond_full, &mutex);
    }

    buffer[in] = i;
    in = (in + 1) % BUFFER_SIZE;
    count++;
    printf("Producer %ld produced %d (Count: %d)\n", id, i, count);

    gcond_signal(&cond_empty);
    gmutex_unlock(&mutex);

    // Simulate work
    if (i % 3 == 0)
      gthread_yield();
  }
  printf("Producer finished\n");
}

void consumer(void *arg) {
  long id = (long)arg;
  for (int i = 0; i < ITEMS_TO_PRODUCE; i++) {
    gmutex_lock(&mutex);
    while (count == 0) {
      // Buffer empty, wait
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
  printf("Consumer finished\n");
}

int main(void) {
  gthread_init();

  gmutex_init(&mutex);
  gcond_init(&cond_full);
  gcond_init(&cond_empty);

  gthread_t *prod, *cons;
  gthread_create(&prod, producer, (void *)1);
  gthread_create(&cons, consumer, (void *)1);

  gthread_join(prod, NULL);
  gthread_join(cons, NULL);

  printf("Main: All threads joined. Exiting.\n");
  return 0;
}
