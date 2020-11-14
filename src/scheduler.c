#include "scheduler.h"

int main(int argc, char *argv[]) {
    task_t *list[3];
    list[0] = malloc(sizeof(task_t));
    list[1] = malloc(sizeof(task_t));
    list[2] = malloc(sizeof(task_t));
    list[0]->comp_time = 20;
    list[0]->deadline = 100;
    list[0]->period = 100;
    list[1]->comp_time = 30;
    list[1]->deadline = 145;
    list[1]->period = 145;
    list[2]->comp_time = 68;
    list[2]->deadline = 150;
    list[2]->period = 150;
    
    qsort(list, 3, sizeof(task_t *), d_comparator);

    uint32_t *returnSize = malloc(4);
    task_t **schedule;

    int rms_results = rms(list, 3, schedule, returnSize);
    
    for (int i = 0; i < *returnSize; i++) {
        if (schedule[i] == NULL) printf("-\n");
        for (int j = 0; j < 3; j++) {
            if (schedule[i] == list[j]) printf("%d\n", j+1);
        }
    }

    return rms_results;
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

int rms(task_t **tasks, uint8_t num_tasks, task_t **schedule, uint32_t *schedule_len) {
    if (exact_analysis(tasks, num_tasks, 0) == 1) return 1;

    *schedule_len = find_lcm(tasks, num_tasks);
    schedule = malloc(sizeof(task_t *) * (*schedule_len));
    uint8_t occupied_spots[*schedule_len] = { 0 };
    int i = 0;

    for (; i < num_tasks; i++) {
        int j = 0;
        for (; j < *schedule_len; j++) {
            int k = j;
            int comp_time_left = tasks[i]->comp_time;

            while (comp_time_left > 0) {
                printf("hello\n");
                if (occupied_spots[k] == 0) {
                    occupied_spots[k] = 1;
                    schedule[k] = tasks[i];
                    comp_time_left--;
                }
            }
            j += tasks[i]->period;
        }
    }

    return 0;
}