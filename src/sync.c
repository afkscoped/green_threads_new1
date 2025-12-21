#include "sync.h"
#include "gthread.h"
#include "scheduler.h"
#include <stdio.h> // For debug
#include <stdlib.h>


/* Internal helper: Add thread to a waiting list (not ready queue) */
static void wait_list_enqueue(gthread_t **head, gthread_t *t) {
  t->next = NULL;
  if (!*head) {
    *head = t;
  } else {
    gthread_t *curr = *head;
    while (curr->next)
      curr = curr->next;
    curr->next = t;
  }
}

static gthread_t *wait_list_dequeue(gthread_t **head) {
  if (!*head)
    return NULL;
  gthread_t *t = *head;
  *head = t->next;
  t->next = NULL;
  return t;
}

/* Mutex */
void gmutex_init(gmutex_t *m) {
  m->locked = 0;
  m->wait_queue = NULL;
}

void gmutex_lock(gmutex_t *m) {
  // Cooperative: No atomics needed if we assume running on one core
  while (m->locked) {
    gthread_t *cur = g_current_thread;
    cur->state = GTHREAD_BLOCKED;
    wait_list_enqueue(&m->wait_queue, cur);
    scheduler_schedule(); // Yield
  }
  m->locked = 1;
}

void gmutex_unlock(gmutex_t *m) {
  if (m->wait_queue) {
    gthread_t *t = wait_list_dequeue(&m->wait_queue);
    scheduler_enqueue(t); // Make it ready
  }
  m->locked = 0; // Wait, if we unblock someone, they will retry lock loop.
                 // But if we set locked=0, any stride scheduled thread could
                 // grab it. This is fair/correct. The unblocked thread needs to
                 // be scheduled first.
}

/* Cond Var */
void gcond_init(gcond_t *c) { c->wait_queue = NULL; }

void gcond_wait(gcond_t *c, gmutex_t *m) {
  gmutex_unlock(m);

  gthread_t *cur = g_current_thread;
  cur->state = GTHREAD_BLOCKED;
  wait_list_enqueue(&c->wait_queue, cur);
  scheduler_schedule();

  gmutex_lock(m);
}

void gcond_signal(gcond_t *c) {
  if (c->wait_queue) {
    gthread_t *t = wait_list_dequeue(&c->wait_queue);
    scheduler_enqueue(t);
  }
}

void gcond_broadcast(gcond_t *c) {
  while (c->wait_queue) {
    gthread_t *t = wait_list_dequeue(&c->wait_queue);
    scheduler_enqueue(t);
  }
}
