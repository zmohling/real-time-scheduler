#include "scheduler.h"

#include <ctype.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int help_flag;

static char *input_file_path, *output_file_path, *algorithm;

void print_usage() {
  puts("usage: scheduler -a ALGO [-o OUTPUTFILE] INPUTFILE");
  exit(1);
}

int main(int argc, char **argv) {
  int c;

  while (1) {
    static struct option long_options[] = {
        {"help", no_argument, &help_flag, 1},
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

  printf("Num tasks: %d\n", num_tasks);

  for (int i = 0; i < num_tasks; i++) {
    printf("Task %d\nComp Time: %d\nPeriod: %d\nDeadline: %d\n", tasks[i]->id,
           tasks[i]->comp_time, tasks[i]->period, tasks[i]->deadline);
  }

  exit(0);
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

// int main(int argc, char *argv[]) {}
