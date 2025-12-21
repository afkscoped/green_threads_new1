#include "monitor.h"
#include "sync.h"
#include <stdio.h>
#include <string.h>

#define MAX_TASKS 256

static monitor_task_t tasks[MAX_TASKS];
static int next_id = 1;
static gmutex_t monitor_mutex;

void monitor_init(void) {
  gmutex_init(&monitor_mutex);
  memset(tasks, 0, sizeof(tasks));
}

int monitor_register(const char *type) {
  gmutex_lock(&monitor_mutex);
  for (int i = 0; i < MAX_TASKS; i++) {
    if (tasks[i].id == 0) {
      tasks[i].id = next_id++;
      tasks[i].state = TASK_RUNNABLE;
      strncpy(tasks[i].type, type, sizeof(tasks[i].type) - 1);
      tasks[i].progress = 0;
      tasks[i].wake_ms = -1;
      tasks[i].wait_fd = -1;
      gmutex_unlock(&monitor_mutex);
      return tasks[i].id;
    }
  }
  gmutex_unlock(&monitor_mutex);
  return -1;
}

static int find_task_index(int id) {
  for (int i = 0; i < MAX_TASKS; i++) {
    if (tasks[i].id == id)
      return i;
  }
  return -1;
}

void monitor_update_state(int id, task_state_t state) {
  gmutex_lock(&monitor_mutex);
  int idx = find_task_index(id);
  if (idx >= 0) {
    tasks[idx].state = state;
  }
  gmutex_unlock(&monitor_mutex);
}

void monitor_update_progress(int id, int progress) {
  gmutex_lock(&monitor_mutex);
  int idx = find_task_index(id);
  if (idx >= 0) {
    tasks[idx].progress = progress;
  }
  gmutex_unlock(&monitor_mutex);
}

void monitor_set_wake(int id, long wake_ms) {
  gmutex_lock(&monitor_mutex);
  int idx = find_task_index(id);
  if (idx >= 0) {
    tasks[idx].wake_ms = wake_ms;
  }
  gmutex_unlock(&monitor_mutex);
}

void monitor_set_waitfd(int id, int fd) {
  gmutex_lock(&monitor_mutex);
  int idx = find_task_index(id);
  if (idx >= 0) {
    tasks[idx].wait_fd = fd;
  }
  gmutex_unlock(&monitor_mutex);
}

void monitor_set_extra(int id, const char *msg) {
  gmutex_lock(&monitor_mutex);
  int idx = find_task_index(id);
  if (idx >= 0) {
    strncpy(tasks[idx].extra, msg, sizeof(tasks[idx].extra) - 1);
    tasks[idx].extra[sizeof(tasks[idx].extra) - 1] = '\0';
  }
  gmutex_unlock(&monitor_mutex);
}

void monitor_mark_done(int id) {
  gmutex_lock(&monitor_mutex);
  int idx = find_task_index(id);
  if (idx >= 0) {
    tasks[idx].state = TASK_DONE;
  }
  gmutex_unlock(&monitor_mutex);
}

int monitor_build_json(char *buf, int buflen) {
  gmutex_lock(&monitor_mutex);

  int offset = 0;
  if (buflen < 3) { // Minimum "[]" + null
    gmutex_unlock(&monitor_mutex);
    return -1;
  }

  buf[offset++] = '[';
  int first = 1;

  for (int i = 0; i < MAX_TASKS; i++) {
    if (tasks[i].id != 0) {
      if (!first) {
        if (offset + 1 >= buflen)
          break;
        buf[offset++] = ',';
      }
      first = 0;

      int remaining = buflen - offset - 2;
      if (remaining <= 0)
        break;

      int wrote = snprintf(
          buf + offset, remaining,
          "{\"id\":%d,\"state\":%d,\"type\":\"%s\",\"progress\":%d,"
          "\"wake_ms\":%ld,\"wait_fd\":%d,\"extra\":\"%s\"}",
          tasks[i].id, tasks[i].state, tasks[i].type, tasks[i].progress,
          tasks[i].wake_ms, tasks[i].wait_fd, tasks[i].extra);

      if (wrote < 0 || wrote >= remaining) {
        break;
      }
      offset += wrote;
    }
  }

  if (offset < buflen - 1) {
    buf[offset++] = ']';
    buf[offset] = '\0';
  } else {
    buf[buflen - 2] = ']';
    buf[buflen - 1] = '\0';
  }

  gmutex_unlock(&monitor_mutex);
  return offset;
}
