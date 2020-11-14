#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <stdint.h>

typedef struct task {
  uint8_t id;
  uint8_t comp_time;
  uint8_t period;
  uint8_t deadline;
} task_t;

int parse_tasks(char *t, task_t ***tasks, uint8_t *num_tasks);

int rms(task_t **tasks, uint8_t num_tasks, task_t **schedule,
        uint32_t *schedule_len);

#endif
