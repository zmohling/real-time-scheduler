#include "scheduler.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[]) {

}

/*
 * Comparator used to sort by period
 */
int p_comparator(const void *p, const void *q) { 
    task_t *l = *(task_t **)p; 
    task_t *r = *(task_t **)q; 
    
    return l->period - r->period;
} 

/*
 * Comparator used to sort by deadline 
 */
int d_comparator(const void *p, const void *q) { 
    task_t *l = *(task_t **)p; 
    task_t *r = *(task_t **)q; 
    
    return l->deadline - r->deadline;
} 


int exact_analysis(task_t **tasks, uint8_t num_tasks, uint8_t is_dms) {
    int i = 0;

    for (; i < num_tasks; i++) {
        int cur_time = 0;
        int end_time;
        int last_cur_time = cur_time;

        if (is_dms) {
            end_time = tasks[num_tasks - i - 1]->deadline;
        } else {
            end_time = tasks[num_tasks - i - 1]->period;
        }
        
        while (1) {
            int j = 0;
    
            if (cur_time == 0) {
                for (; j < num_tasks - i; j++) {
                    cur_time += tasks[j]->comp_time;
                }
            } else {
                last_cur_time = cur_time;
                cur_time = 0;
                for (; j < num_tasks - i; j++) {

                    int num_instances = ceil((double) last_cur_time / (double) tasks[j]->period);
                    
                    cur_time += tasks[j]->comp_time * num_instances;
                }
            }

            if (cur_time > end_time) return 1;
            else if (last_cur_time == cur_time) break;
        }
    }

    return 0;
}

uint32_t gcd(int a, int b) 
{ 
    if (b == 0) 
        return a; 
    return gcd(b, a % b); 
} 

uint32_t find_lcm(task_t **tasks, uint8_t num_tasks) {
    uint32_t lcm = tasks[0]->period;
    int i = 1;

    for (; i < num_tasks; i++) {
        lcm = (((tasks[i]->period * lcm)) / (gcd(tasks[i]->period, lcm)));
    }

    return lcm;
} 

int ms(task_t **tasks, uint8_t num_tasks, task_t ***schedule, uint32_t *schedule_len, uint8_t is_dms) {
    if (exact_analysis(tasks, num_tasks, 0) == 1) return 1;

    current_task_state_t task_state[num_tasks];
    int i = 0;
    *schedule_len = find_lcm(tasks, num_tasks);
    schedule[0] = malloc(sizeof(task_t *) * (*schedule_len));

    if (is_dms) {
        for (; i < num_tasks; i++) {
            task_state[i].priority = tasks[i]->deadline;
            //task_state[i].comp_time_left = tasks[i]->comp_time;
        }
    } else {
        for (; i < num_tasks; i++) {
            task_state[i].priority = tasks[i]->period;
            //task_state[i].comp_time_left = tasks[i]->comp_time;
        }
    }

    for (i = 0; i < (*schedule_len); i++) {
        int j = 0;
        int highest_schedulable_priority = -1;
        task_t *highest_priority_task = NULL;
        int task_index = -1;

        for (; j < num_tasks; j++) {
            //check if new period to reset comp time
            if (i % tasks[j]->period == 0) {
                task_state[j].comp_time_left = tasks[j]->comp_time;
            }
    
            //find highest priority task that still has comp time
            if ((task_state[j].priority < highest_schedulable_priority || 
                highest_schedulable_priority == -1) && 
                task_state[j].comp_time_left > 0) {
                    highest_schedulable_priority = task_state[j].priority;
                    highest_priority_task = tasks[j];
                    task_index = j;
            }
        }

        schedule[0][i] = highest_priority_task;
        if (task_index != -1) task_state[task_index].comp_time_left--;
        //printf("Task: %d, Time Left: %d\n", task_index, task_state[task_index].comp_time_left);
    }

    return 0;
}

int utilization_test(task_t **tasks, uint8_t num_tasks) {
    int i = 0;
    double utilization_sum = 0;
    for (; i < num_tasks; i++) {
        utilization_sum += tasks[i]->comp_time / tasks[i]->period;
    }

    if (utilization_sum > 1) return 1;
    return 0;
}

int edf(task_t **tasks, uint8_t num_tasks, task_t ***schedule, uint32_t *schedule_len) {
    if (utilization_test(tasks, num_tasks) == 1) return 1;

    current_task_state_t task_state[num_tasks];
    int i = 0;
    *schedule_len = find_lcm(tasks, num_tasks);
    schedule[0] = malloc(sizeof(task_t *) * (*schedule_len));

    for (; i < (*schedule_len); i++) {
        int j = 0;
        int highest_schedulable_priority = -1;
        task_t *highest_priority_task = NULL;
        int task_index = -1;

        for (; j < num_tasks; j++) {
            //check if new period to reset comp time
            if (i % tasks[j]->period == 0) {
                task_state[j].comp_time_left = tasks[j]->comp_time;
                task_state[j].priority = tasks[j]->period + i;
            }
    
            //find highest priority task that still has comp time
            if ((task_state[j].priority < highest_schedulable_priority || 
                highest_schedulable_priority == -1) && 
                task_state[j].comp_time_left > 0) {
                    highest_schedulable_priority = task_state[j].priority;
                    highest_priority_task = tasks[j];
                    task_index = j;
            }
        }

        schedule[0][i] = highest_priority_task;
        if (task_index != -1) task_state[task_index].comp_time_left--;        
    }
    return 0;
}

int llf(task_t **tasks, uint8_t num_tasks, task_t ***schedule, uint32_t *schedule_len) {
    if (utilization_test(tasks, num_tasks) == 1) return 1;

    current_task_state_t task_state[num_tasks];
    int next_deadline[num_tasks];
    int i = 0;
    *schedule_len = find_lcm(tasks, num_tasks);
    schedule[0] = malloc(sizeof(task_t *) * (*schedule_len));

    for (; i < (*schedule_len); i++) {
        int j = 0;
        int highest_schedulable_priority = -1;
        task_t *highest_priority_task = NULL;
        int task_index = -1;

        for (; j < num_tasks; j++) {
            task_state[j].priority = next_deadline[j] - (i + task_state[j].comp_time_left);
            //check if new period to reset comp time
            if (i % tasks[j]->period == 0) {
                task_state[j].comp_time_left = tasks[j]->comp_time;
                next_deadline[j] = tasks[j]->period + i;
            }
    
            //find highest priority task that still has comp time
            if ((task_state[j].priority < highest_schedulable_priority || 
                highest_schedulable_priority == -1) && 
                task_state[j].comp_time_left > 0) {
                    highest_schedulable_priority = task_state[j].priority;
                    highest_priority_task = tasks[j];
                    task_index = j;
            }
        }

        schedule[0][i] = highest_priority_task;
        if (task_index != -1) task_state[task_index].comp_time_left--;        
    }
    return 0;
}