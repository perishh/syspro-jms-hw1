#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#define JMS_IN "jms_in"
#define JMS_OUT "jms_out"

// chmod(2)
// TODO: Recheck (maybe just user?)
#define MODE_RW S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH

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

  // Change working directory to path
  // chdir(2)
  // TODO: Maybe not needed, possibly erroneous
  if (chdir(path) < 0) {
    perror("chdir");
    return 1;
  }

  // Clear leftover files
  // TODO: Recheck (add more)
  unlink(JMS_IN);
  unlink(JMS_OUT);

  // Create fifos
  // mkfifo(3)
  if (mkfifo(JMS_IN, MODE_RW) < 0) {
    perror("mkfifo (in)");
    return 1;
  }

  if (mkfifo(JMS_OUT, MODE_RW) < 0) {
    perror("mkfifo (out)");

    unlink(JMS_IN);
    return 1;
  }
}