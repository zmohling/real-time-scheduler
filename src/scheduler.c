#include "scheduler.h"

#include <ctype.h>
#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *strupper(char *str) {
  char *upper = (char *)malloc(strlen(str) + 1);
  int i;
  for (i = 0; str[i] != '\0'; i++) {
    upper[i] = toupper(str[i]);
  }
  upper[i] = '\0';

  return upper;
}

static char *input_file_path, *output_file_path, *algorithm;

int main(int argc, char **argv) {
  int c;

  while (1) {
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"algorithm", required_argument, 0, 'a'},
        {"output", required_argument, 0, 'o'},
        {0, 0, 0, 0}};

    int option_index = 0;

    c = getopt_long(argc, argv, "ha:o:", long_options, &option_index);

    /* Detect the end of the options. */
    if (c == -1) break;

    switch (c) {
      case 0:
        /* If this option set a flag, do nothing else now. */
        if (long_options[option_index].flag != 0) break;
        printf("option %s", long_options[option_index].name);
        if (optarg) printf(" with arg %s", optarg);
        printf("\n");
        break;
      case 'a':
        algorithm = optarg;
        break;
      case 'o':
        output_file_path = optarg;
        break;
      case 'h':
      case '?':
        print_usage();
        break;
      default:
        printf("unknown option: `%s'\n", optarg);
        abort();
    }
  }

  if (algorithm == NULL) {
    printf("Expected algorithm argument\n");
    print_usage();
  }

  /* Print any remaining command line arguments (not options). */
  if (optind + 1 < argc) {
    printf("Unrecognized elements: ");
    while (optind + 1 < argc) printf("%s ", argv[optind++ + 1]);
    putchar('\n');
    print_usage();
  } else if (optind == argc) {
    printf("Missing argument: INPUTFILE\n");
    print_usage();
  } else {
    input_file_path = argv[optind];
  }

  FILE *input_file = fopen(input_file_path, "r+");
  char *task_text_buf = (char *)malloc(255);
  fgets(task_text_buf, 255, input_file);

  task_t **tasks = NULL;
  uint8_t num_tasks = 0;

  parse_tasks(task_text_buf, &tasks, &num_tasks);

  task_t **schedule = NULL;
  uint32_t schedule_len = 0;

  char *a = strupper(algorithm);
  if (strcmp(a, "RMS") == 0) {
    ms(tasks, num_tasks, &schedule, &schedule_len, 0);
  } else if (strcmp(a, "LLF") == 0) {
    llf(tasks, num_tasks, &schedule, &schedule_len);
  } else if (strcmp(a, "EDF") == 0) {
    edf(tasks, num_tasks, &schedule, &schedule_len);
  } else if (strcmp(a, "DMS") == 0) {
    ms(tasks, num_tasks, &schedule, &schedule_len, 1);
  } else {
    printf("%s algorithm unrecognized\n", a);
    print_usage();
  }

  FILE *output_file;
  if (output_file_path != NULL) {
    output_file = fopen(output_file_path, "w+");
  }

  FILE *out_file = (output_file_path) ? output_file : stdout;
  for (int i = 0; i < schedule_len; i++) {
    if (schedule[i]) {
      fprintf(out_file, "%.2d: Task %d\n", i + 1, schedule[i]->id);
    } else {
      fprintf(out_file, "%d: -\n", i + 1);
    }
  }

  exit(0);
}

void print_usage() {
  puts("usage: scheduler -a ALGO [-o OUTPUTFILE] INPUTFILE");
  exit(1);
}

int parse_tasks(char *t, task_t ***tasks, uint8_t *num_tasks) {
  *tasks = (task_t **)malloc(sizeof(task_t **) * 1);

  int curr_task = 0;

  while (*t != '\0') {
    if (*t == ' ') {
      t++;
      continue;
    }

    if (toupper(*t) == 'T') {
      *num_tasks = *num_tasks + 1;

      if (*num_tasks > 1) {
        *tasks = (task_t **)realloc(*tasks, sizeof(task_t **) * *num_tasks);
      }

      (*tasks)[*num_tasks - 1] = (task_t *)malloc(sizeof(task_t *));

      (*tasks)[*num_tasks - 1]->id = t[2] - 48;
    } else if (*t == '(') {
      /* Get (c_i, p_i, d_i) */
      char *ptr = t;
      while (*ptr != ')') {
        ptr++;
      }

      int arg_num = 0;  // tracks which argument we're parsing

      /* Tokenize */
      char *token, *str = strndup(t + 1, ptr - t - 1);
      while ((token = strsep(&str, ","))) {
        switch (arg_num) {
          case 0:
            (*tasks)[curr_task]->comp_time = atoi(token);
            break;
          case 1:
            (*tasks)[curr_task]->period = atoi(token);
            break;
          case 2:
            (*tasks)[curr_task]->deadline = atoi(token);
            break;
        }
        arg_num++;
      }

      if (arg_num < 3) {
        (*tasks)[curr_task]->deadline = (*tasks)[curr_task]->period;
      }

      curr_task++;
    }

    t++;
  }

  return 1;
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
          int num_instances =
              ceil((double)last_cur_time / (double)tasks[j]->period);

          cur_time += tasks[j]->comp_time * num_instances;
        }
      }

      if (cur_time > end_time)
        return 1;
      else if (last_cur_time == cur_time)
        break;
    }
  }

  return 0;
}

uint32_t gcd(int a, int b) {
  if (b == 0) return a;
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

int ms(task_t **tasks, uint8_t num_tasks, task_t ***schedule,
       uint32_t *schedule_len, uint8_t is_dms) {
  if (is_dms)
    qsort(tasks, num_tasks, sizeof(task_t *), d_comparator);
  else
    qsort(tasks, num_tasks, sizeof(task_t *), p_comparator);

  if (exact_analysis(tasks, num_tasks, 0) == 1) return 1;

  current_task_state_t task_state[num_tasks];
  int i = 0;
  *schedule_len = find_lcm(tasks, num_tasks);
  schedule[0] = malloc(sizeof(task_t *) * (*schedule_len));

  if (is_dms) {
    for (; i < num_tasks; i++) {
      task_state[i].priority = tasks[i]->deadline;
      // task_state[i].comp_time_left = tasks[i]->comp_time;
    }
  } else {
    for (; i < num_tasks; i++) {
      task_state[i].priority = tasks[i]->period;
      // task_state[i].comp_time_left = tasks[i]->comp_time;
    }
  }

  for (i = 0; i < (*schedule_len); i++) {
    int j = 0;
    int highest_schedulable_priority = -1;
    task_t *highest_priority_task = NULL;
    int task_index = -1;

    for (; j < num_tasks; j++) {
      // check if new period to reset comp time
      if (i % tasks[j]->period == 0) {
        task_state[j].comp_time_left = tasks[j]->comp_time;
      }

      // find highest priority task that still has comp time
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
    // printf("Task: %d, Time Left: %d\n", task_index,
    // task_state[task_index].comp_time_left);
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

int edf(task_t **tasks, uint8_t num_tasks, task_t ***schedule,
        uint32_t *schedule_len) {
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
      // check if new period to reset comp time
      if (i % tasks[j]->period == 0) {
        task_state[j].comp_time_left = tasks[j]->comp_time;
        task_state[j].priority = tasks[j]->period + i;
      }

      // find highest priority task that still has comp time
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

int llf(task_t **tasks, uint8_t num_tasks, task_t ***schedule,
        uint32_t *schedule_len) {
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
      task_state[j].priority =
          next_deadline[j] - (i + task_state[j].comp_time_left);
      // check if new period to reset comp time
      if (i % tasks[j]->period == 0) {
        task_state[j].comp_time_left = tasks[j]->comp_time;
        next_deadline[j] = tasks[j]->period + i;
      }

      // find highest priority task that still has comp time
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
