#ifndef MONITOR_H
#define MONITOR_H

#include <stdint.h>

typedef enum {
  TASK_RUNNABLE = 0,
  TASK_SLEEPING = 1,
  TASK_WAITING = 2,
  TASK_DONE = 3
} task_state_t;

typedef struct {
  int id; // small unique id
  task_state_t state;
  char type[32]; // "CPU", "TIMER", "IO", "GTHREAD"
  int progress;  // 0-100 for CPU tasks (optional)
  long wake_ms;  // if sleeping, when it will wake (epoch ms) or -1
  int wait_fd;   // if waiting on fd
} monitor_task_t;

/* library init */
void monitor_init(void);

/* register/unregister a task; returns unique id */
int monitor_register(const char *type);

/* update fields (progress, state, etc) */
void monitor_update_state(int id, task_state_t state);
void monitor_update_progress(int id, int progress);
void monitor_set_wake(int id, long wake_ms);
void monitor_set_waitfd(int id, int fd);

/* mark done */
void monitor_mark_done(int id);

/* produce JSON into provided buffer; returns bytes written or -1 */
int monitor_build_json(char *buf, int buflen);

#endif
