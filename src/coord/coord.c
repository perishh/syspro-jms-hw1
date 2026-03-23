#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void print_usage() { printf("Usage: jms_coord -l <path> -n <jobs_pool>"); }

int main(int argc, char **argv) {
  char *path = NULL;
  int jobs_pool = 0;

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
      return 1;
    }
  }

  if (path == NULL || jobs_pool <= 0) {
    // Ensure arguments are valid
    print_usage();
    return 1;
  }
}