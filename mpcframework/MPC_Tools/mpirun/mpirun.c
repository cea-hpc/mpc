#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

void display_help() {
  printf("mpirun MPC\n"
         "USAGE: mpirun [OPTION]... [PROGRAM]...\n\n"
         "\t-n|-p\t\tNumber of processes to launch\n"
         "\t-c\t\tNumber of cores per process\n"
         "\t--verbose\tEnable debug messages.\n"
         "\n");
}

static int verbose_flag = 0;
static int nb_process = -1;
static int nb_cores = -1;

int main(int argc, char **argv) {
  int nb_process = 0;
  int nb_core = 0;
  int c;
  int done = 0;

  int i;

  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-np")) {
      argv[i] = "-p";
    }

    if (!strcmp(argv[i], "--")) {
      break;
    }
  }

  while (1) {
    static struct option long_options[] = {
        /* These options set a flag. */
        {"verbose", no_argument, &verbose_flag, 1},
        {"brief", no_argument, &verbose_flag, 0},
        {"processes", required_argument, 0, 'p'},
        {"np", required_argument, 0, 'n'},
        {"cores", required_argument, 0, 'c'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}};
    /* getopt_long stores the option index here. */
    int option_index = 0;

    c = getopt_long(argc, argv, "+hp:n:c:", long_options, &option_index);

    /* Detect the end of the options. */
    if (c == -1)
      break;

    switch (c) {
    case 'p':
    case 'n':
      nb_process = atoi(optarg);
      break;
    case 'c':
      nb_cores = atoi(optarg);
      break;
    case 'h':
      display_help();
      return 0;
      break;
    default:
      done = 1;
      break;
    }

    /* Encountered unknown arg */
    if (done)
      break;
  }

  char *command = malloc(4 * 1024 * sizeof(char));

  if (!command) {
    perror("malloc");
    return 1;
  }

  command[0] = '\0';

  char tmp[128];

  strcat(command, "mpcrun ");

  char* nodes = getenv("SLURM_JOB_NUM_NODES");

  if(nodes)
  {
    snprintf(tmp, 128, " -N=%s ", nodes);
    strcat(command, tmp);
  }

  if (0 < nb_process) {
    snprintf(tmp, 128, " -p=%d ", nb_process);
    strcat(command, tmp);
  }

  if (0 < nb_process) {
    snprintf(tmp, 128, "-n=%d ", nb_process);
    strcat(command, tmp);
  }

  if (0 < nb_cores) {
    snprintf(tmp, 128, "-c=%d ", nb_cores);
    strcat(command, tmp);
  }

  if (verbose_flag) {
    strcat(command, " -vvv");
  }

  for (i = optind; i < argc; i++) {
    strcat(command, " ");
    strcat(command, argv[i]);
  }



  printf("%s\n", command);

  system(command);

  free(command);

  return 0;
}
