#include "scheduler.h"

#include <ctype.h>
#include <getopt.h>
#include <limits.h>
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
  } else if (strcmp(a, "LLF") == 0) {
  } else if (strcmp(a, "EDF") == 0) {
  } else if (strcmp(a, "DMS") == 0) {
  } else {
    printf("%s algorithm unrecognized\n", a);
    print_usage();
  }

  /*
  printf("Num tasks: %d\n", num_tasks);

  for (int i = 0; i < num_tasks; i++) {
    printf("Task %d\nComp Time: %d\nPeriod: %d\nDeadline: %d\n", tasks[i]->id,
           tasks[i]->comp_time, tasks[i]->period, tasks[i]->deadline);
  }
  */

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
