#ifndef GREEN_THREAD_H
#define GREEN_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

/* Mode constants */
#define MODE_GREEN 0
#define MODE_PTHREAD 1

/* Types */
typedef void (*gt_fn)(void *arg);
typedef int gt_tid;

typedef struct gt_mutex gt_mutex_t;
typedef struct gt_cond gt_cond_t;

/* Initialization */
int gt_init(int mode); // MODE_GREEN or MODE_PTHREAD

/* Thread Management */
gt_tid gt_create(gt_fn fn, void *arg, size_t stack_size, int priority);
void gt_yield(void);
void gt_join(gt_tid tid);
void gt_exit(void); // Explicit exit
void gt_sleep_ms(int ms);
int gt_get_mode(void); // Helper to check current mode

/* Synchronization */
// Opaque structs defined effectively in impl, but here we might need pointers
// or fixed size if stack allocated. For simplicity, we'll use a pointer handle
// or a struct with padding. Actually, let's define the struct size or use
// pointers. The prompt implies `gt_mutex_t *m`, so we'll define a typedef. We
// will need a create/alloc function or expose the struct. "int
// gt_mutex_init(gt_mutex_t *m);" implies caller allocates it. So we must define
// the struct content or a large enough buffer.
struct gt_mutex {
  void *internal; // Pointer to implementation-specific data (green vs pthread)
  int id;         // Track for debugging
};

struct gt_cond {
  void *internal;
  int id;
};

int gt_mutex_init(gt_mutex_t *m);
int gt_mutex_lock(gt_mutex_t *m);
int gt_mutex_unlock(gt_mutex_t *m);
int gt_mutex_destroy(gt_mutex_t *m);

int gt_cond_init(gt_cond_t *c);
int gt_cond_wait(gt_cond_t *c, gt_mutex_t *m);
int gt_cond_signal(gt_cond_t *c);
int gt_cond_broadcast(gt_cond_t *c);
int gt_cond_destroy(gt_cond_t *c);

/* Scheduler Configuration */
void gt_set_stride_tickets(gt_tid tid, int tickets);
void gt_set_global_timeslice_ms(int ms);
void gt_set_scheduler_type(int type); // 0=RR, 1=Stride

extern int g_global_timeslice_ms;
extern int g_default_tickets;

#ifdef __cplusplus
}
#endif

#endif // GREEN_THREAD_H
