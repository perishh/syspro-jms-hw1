#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

void print_usage() {
  fprintf(stderr, "Usage: jms_console -w <jms_in> -r <jms_out> [-o "
                  "<operations_file>]\n");
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

  // Open jms_in to send data to coord
  int out = open(jms_in, O_WRONLY);
  if(out < 0) {
    perror("open (jms_in)");
    return 1;
  }

  // Allocate buffer
  char* buffer = malloc(BUFFER_SIZE);
  if(buffer == NULL) {
    perror("malloc");
    close(out);
    return 1;
  }

  // Forward from stdin to jms_in
  ssize_t nread;
  while((nread = read(STDIN_FILENO, buffer, BUFFER_SIZE)) > 0) {
    write(out, buffer, nread);
  }

  if(nread < 0) {
    perror("read (stdin)");
    free(buffer);
    close(out);
    return 1;
  }

  return 0;
}