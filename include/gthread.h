#ifndef GTHREAD_H
#define GTHREAD_H

#include <stddef.h>
#include <stdint.h>

/* Thread States */
typedef enum {
  GTHREAD_NEW,
  GTHREAD_READY,
  GTHREAD_RUNNING,
  GTHREAD_BLOCKED,
  GTHREAD_TERMINATED
} gthread_state_t;

/* Thread Handle */
typedef struct gthread gthread_t;

/* Context Structure (Architecture Dependent - x86_64) */
typedef struct {
  uint64_t rbx;
  uint64_t rbp;
  uint64_t r12;
  uint64_t r13;
  uint64_t r14;
  uint64_t r15;
  uint64_t rip;
  uint64_t rsp;
} gthread_ctx_t;

/* Thread Control Block */
struct gthread {
  uint64_t id;
  gthread_ctx_t ctx;
  void *stack;
  size_t stack_size;
  gthread_state_t state;
  void (*entry)(void *);
  void *arg;
  struct gthread *next; /* For queueing */

  // Stride scheduling fields (Phase 3)
  uint64_t tickets;
  uint64_t stride;
  uint64_t pass;

  // Phase 5: Join support
  struct gthread *join_queue;
  void *retval;

  // Phase 6: Sleep
  uint64_t wake_time_ms;
};

/* API */
int gthread_create(gthread_t **t, void (*fn)(void *), void *arg);
void gthread_exit(void);
void gthread_yield(void);
int gthread_join(gthread_t *t, void **retval);
void gthread_sleep(uint64_t ms);

/* Internal Init (Call once) */
void gthread_init(void);

#endif
