#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "command.h"

#define INITIAL_BUFFER_SIZE 4096

void print_usage() {
  fprintf(stderr, "Usage: jms_console -w <jms_in> -r <jms_out> [-o "
                  "<operations_file>]\n");
}

int out;
Command *cmd = NULL;

char *buffer = NULL;
size_t buffer_size = 0;

Action parse_action(const char *cmd);
long read_commands(FILE *stream);
long redirect(int fromfd, int tofd);

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
      /* fallthrough */
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

  int in = open(jms_out, O_RDONLY | O_NONBLOCK);
  if (in < 0) {
    if (errno == ENXIO) {
      fprintf(stderr, "Coordinator is not running.\n");
    } else {
      perror("open (jms_out)");
    }
    close(out);
    return 1;
  }

  buffer_size = INITIAL_BUFFER_SIZE;
  buffer = malloc(INITIAL_BUFFER_SIZE);
  if (buffer == NULL) {
    perror("malloc");
    close(in);
    close(out);
    return 1;
  }

  cmd = malloc(sizeof(Command));
  if (cmd == NULL) {
    perror("malloc");
    free(buffer);
    close(in);
    close(out);
    return 1;
  }

  if (operations_file != NULL) {
    FILE *ops = fopen(operations_file, "r");
    if (ops == NULL) {
      // ferror(3)
      perror("fopen");
      // Program can continue
    } else {
      // While there still is unread data
      while (read_commands(ops) > 0) {
      }
      fclose(ops);
    }
  }

  // poll(2)
  struct pollfd fds[2];
  fds[0].fd = STDIN_FILENO;
  fds[0].events = POLLIN;

  fds[1].fd = in;
  fds[1].events = POLLIN;

  for (;;) {
    int ret = poll(fds, 2, -1);
    if (ret > 0) {
      if (fds[0].revents & POLLIN) {
        if (read_commands(stdin) >= 0) {
        }
      }
      if (fds[1].revents & POLLIN) {
        if (redirect(in, STDOUT_FILENO) == -2) {
          break;
        }
      }
    } else {
      perror("poll");
      break;
    }
  }

  free(cmd);
  free(buffer);
  close(in);
  close(out);

  return 0;
}

long redirect(int fromfd, int tofd) {
  ssize_t nread;
  while ((nread = read(fromfd, buffer, buffer_size)) > 0) {
    if (buffer[0] == 0x04 || buffer[nread - 1] == 0x04) {
      return -2;
    }
    write(tofd, buffer, nread);
  }

  return nread;
}

long read_commands(FILE *stream) {
  ssize_t nread = getline(&buffer, &buffer_size, stream);
  if (nread <= 0) {
    // ferror(3)
    if (feof(stream))
      return 0;
    return nread;
  }

  // Parse command
  char *action = strtok(buffer, " \n");
  if (action == NULL) {
    fprintf(stderr, "Invalid command.\n");
    return -1;
  }

  cmd->action = parse_action(action);
  cmd->len = 0;
  if (cmd->action == UNKNOWN) {
    fprintf(stderr, "Invalid command.\n");
    return -1;
  }

  ssize_t arg_length_with_null = 0;
  ssize_t action_length_with_null = ((long)strlen(action)) + 1;

  if ((cmd->action & ZERO_ARG_ACTIONS) != 0) {
    if (nread > action_length_with_null) {
      // Argument not empty
      fprintf(stderr, "Unknown arguments.\n");
      return -1;
    }
  } else {
    if (nread <= action_length_with_null && cmd->action != STATUS_ALL) {
      // Argument empty
      fprintf(stderr, "Arguments not found.\n");
      return -1;
    }

    arg_length_with_null = nread - action_length_with_null + 1;
    cmd->len = (int)arg_length_with_null - 1;
  }

  // Send command
  ssize_t written = write(out, cmd, sizeof(Command));
  if (written < (long)sizeof(Command)) {
    if (errno == EPIPE) {
      fprintf(stderr, "Coordinator is no longer running.\n");
    }
    return -1;
  }

  if (cmd->len > 0) {
    written =
        write(out, buffer + action_length_with_null, arg_length_with_null);
    if (written < arg_length_with_null) {
      if (errno == EPIPE) {
        fprintf(stderr, "Coordinator is no longer running.\n");
      }
      return -1;
    }
  }

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