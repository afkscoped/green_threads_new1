#ifndef AURA_H
#define AURA_H

#include "internal.h"

/* AURA Configuration */
#define AURA_STARVATION_THRESHOLD_MS 500
#define AURA_OVERSUBSCRIPTION_THRESHOLD 10

/* AURA Events */
typedef enum {
  AURA_EVENT_NORMAL,
  AURA_EVENT_STARVATION,
  AURA_EVENT_OVERSUBSCRIPTION,
  AURA_EVENT_UNFAIRNESS
} aura_event_type_t;

void aura_init(void);
void aura_on_thread_start(green_thread_t *t);
void aura_on_schedule(green_thread_t *t);
void aura_tick(void); // Called periodically

#endif
