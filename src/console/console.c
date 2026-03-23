#include <stdio.h>
#include <unistd.h>

void print_usage() {
  printf("Usage: jms_console -w <jms_in> -r <jms_out> [-o "
         "<operations_file>]");
}

int main(int argc, char **argv) {
  char *jms_in = NULL;
  char *jms_out = NULL;
  char *operations_file = NULL;

  // getopt(3)
  int opt;
  while ((opt = getopt(argc, argv, "w:r:o")) != -1) {
    switch (opt) {
    case 'w':
      jms_in = optarg;
      break;
    case 'r':
      jms_out = optarg;
      break;
    case 'o':
      operations_file = optarg;
      break;
    default:
      // Empty or unknown argument
      print_usage();
      return 1;
    }
  }

  if (jms_in == NULL || jms_out == NULL) {
    // Ensure required parameters were given
    print_usage();
    return 1;
  }
}