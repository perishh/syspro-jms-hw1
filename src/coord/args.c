#include "args.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int jobs_pool;

void print_usage() {
  fprintf(stderr, "Usage: jms_coord -l <path> -n <jobs_pool>\n");
}

int get_jobs_pool() {
  return jobs_pool;
}

int args_init(int argc, char **argv) {
  char *path;

  // getopt(3)
  int opt;
  while ((opt = getopt(argc, argv, "l:n:")) != -1) {
    switch (opt) {
    case 'l':
      path = optarg;
      break;
    case 'n':
      jobs_pool = atoi(optarg);
      break;
    default:
      // Empty or unknown argument
      print_usage();
      return -1;
    }
  }

  if (path == NULL || jobs_pool <= 0) {
    // Ensure arguments are valid
    print_usage();
    return -1;
  }

  // Change working directory to path
  // chdir(2)
  if (chdir(path) < 0) {
    perror("chdir");
    return -1;
  }

  return 0;
}