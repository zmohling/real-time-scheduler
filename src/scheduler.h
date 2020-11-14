#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <math.h>
#include <stdint.h>

typedef struct task {
  uint8_t id;
  uint8_t comp_time;
  uint8_t period;
  uint8_t deadline;
} task_t;

typedef struct current_task_state {
        uint32_t priority;
        uint8_t comp_time_left;
} current_task_state_t;

int parse_tasks(char *t, task_t ***tasks, uint8_t *num_tasks);

int p_comparator(const void *p, const void *q);

int d_comparator(const void *p, const void *q);

int exact_analysis(task_t **tasks, uint8_t num_tasks, uint8_t is_dms);

uint32_t gcd(int a, int b);

uint32_t find_lcm(task_t **tasks, uint8_t num_tasks);

int ms(task_t **tasks, uint8_t num_tasks, task_t ***schedule,
        uint32_t *schedule_len, uint8_t is_dms);

int utilization_test(task_t **tasks, uint8_t num_tasks);

int edf(task_t **tasks, uint8_t num_tasks, task_t ***schedule,
        uint32_t *schedule_len);

int llf(task_t **tasks, uint8_t num_tasks, task_t ***schedule,
        uint32_t *schedule_len);

#endif