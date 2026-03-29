#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "command.h"

#define BUFFER_SIZE 4096

void print_usage() {
  fprintf(stderr, "Usage: jms_console -w <jms_in> -r <jms_out> [-o "
                  "<operations_file>]\n");
}

int out;
Command *cmd = NULL;
char *buffer = NULL;

Action parse_action(const char *cmd);
int read_commands(FILE *stream);

int main(int argc, char **argv) {
  // Ignore SIGPIPE to prevent crashing when writing to closed pipe
  // signal(2), write(2)
  signal(SIGPIPE, SIG_IGN);

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
      // Workaround to allow space after -o
      if (optarg) {
        operations_file = optarg;
        break;
      } else if (optind < argc && argv[optind][0] != '-') {
        operations_file = argv[optind++];
        break;
      }
      [[fallthrough]];
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
  out = open(jms_in, O_WRONLY | O_NONBLOCK);
  if (out < 0) {
    if (errno == ENXIO) {
      fprintf(stderr, "Coordinator is not running.\n");
    } else {
      perror("open (jms_in)");
    }
    return 1;
  }

  cmd = malloc(sizeof(Command));
  if (cmd == NULL) {
    perror("malloc");
    close(out);
    return 1;
  }

  if (operations_file != NULL) {
    FILE *ops = fopen(operations_file, "r");
    if (ops == NULL) {
      perror("fopen");
      // Program can continue
    } else {
      if (read_commands(ops) < 0) {
        perror("read_commands (stdin)");

        free(cmd);
        free(buffer);
        close(out);
        return 1;
      }
      fclose(ops);
    }
  }

  // Forward stdin to jms_in
  if (read_commands(stdin) < 0) {
    perror("read_commands (stdin)");

    free(cmd);
    free(buffer);
    close(out);
    return 1;
  }

  free(cmd);
  free(buffer);
  close(out);

  return 0;
}

int read_commands(FILE *stream) {
  static size_t buffer_size;

  // Potentially unsafe to write more than PIPE_BUF at once
  ssize_t nread;
  while ((nread = getline(&buffer, &buffer_size, stream)) > 0) {
    // Parse command
    char *action = strtok(buffer, " \n");
    if (action == NULL) {
      fprintf(stderr, "Invalid command.\n");
      continue;
    }

    cmd->action = parse_action(action);
    cmd->len = 0;
    if (cmd->action == UNKNOWN) {
      fprintf(stderr, "Invalid command.\n");
      continue;
    }

    ssize_t arg_length_with_null = 0;
    ssize_t action_length_with_null = strlen(action) + 1;

    if ((cmd->action & ZERO_ARG_ACTIONS) != 0) {
      if (nread > action_length_with_null) {
        // Argument not empty
        fprintf(stderr, "Unknown arguments.\n");
        continue;
      }
    } else {
      if (nread <= action_length_with_null && cmd->action != STATUS_ALL) {
        // Argument empty
        fprintf(stderr, "Arguments not found.\n");
        continue;
      }

      arg_length_with_null = nread - action_length_with_null + 1;
      cmd->len = arg_length_with_null - 1;
    }

    // Send command
    if (write(out, cmd, sizeof(Command)) < 0) {
      if (errno == EPIPE) {
        fprintf(stderr, "Coordinator is no longer running.\n");
      }
      return -1;
    }

    if (write(out, buffer + action_length_with_null, arg_length_with_null) <
        0) {
      if (errno == EPIPE) {
        fprintf(stderr, "Coordinator is no longer running.\n");
      }
      return -1;
    }
  }
  // ferror(3)
  // Distinguish end of file and error
  if (feof(stream))
    return 0;
  return nread;
}

Action parse_action(const char *cmd) {
  if (strcmp(cmd, "submit") == 0) {
    return SUBMIT;
  }
  if (strcmp(cmd, "status") == 0) {
    return STATUS;
  }
  if (strcmp(cmd, "status-all") == 0) {
    return STATUS_ALL;
  }
  if (strcmp(cmd, "show-active") == 0) {
    return SHOW_ACTIVE;
  }
  if (strcmp(cmd, "show-pools") == 0) {
    return SHOW_POOLS;
  }
  if (strcmp(cmd, "show-finished") == 0) {
    return SHOW_FINISHED;
  }
  if (strcmp(cmd, "suspend") == 0) {
    return SUSPEND;
  }
  if (strcmp(cmd, "resume") == 0) {
    return SUSPEND;
  }
  if (strcmp(cmd, "suspend") == 0) {
    return RESUME;
  }
  if (strcmp(cmd, "shutdown") == 0) {
    return SHUTDOWN;
  }
  return UNKNOWN;
}