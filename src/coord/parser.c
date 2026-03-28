#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "jobs.h"

void print_usage() {
  fprintf(stderr, "Usage: jms_coord -l <path> -n <jobs_pool>\n");
}

int parse_arguments(int argc, char **argv, char **path, int *jobs_pool) {
  // getopt(3)
  int opt;
  while ((opt = getopt(argc, argv, "l:n:")) != -1) {
    switch (opt) {
    case 'l':
      *path = optarg;
      break;
    case 'n':
      *jobs_pool = atoi(optarg);
      break;
    default:
      // Empty or unknown argument
      print_usage();
      return -1;
    }
  }

  if (*path == NULL || *jobs_pool <= 0) {
    // Ensure arguments are valid
    print_usage();
    return -1;
  }
  
  return 0;
}

int parse_commands(int jms_in, Command *cmd_buffer) {
  static ssize_t buffer_size = sizeof(Command);

  ssize_t nread;
  while ((nread = read(jms_in, cmd_buffer, sizeof(Command))) > 0) {
    if ((cmd_buffer->action & ZERO_ARG_ACTIONS) != 0) {
      // Command doesn't require args
      // TODO: Implement
      continue;
    }

    if (cmd_buffer->len <= 0) {
      // TODO: Throw error, invalid argument size
    }

    // Check current buffer size and expand if needed
    ssize_t required_space = sizeof(Command) + cmd_buffer->len + 1;
    if (buffer_size < required_space) {
      buffer_size = required_space;

      Command *temp = realloc(cmd_buffer, required_space);
      if (temp == NULL) {
        perror("realloc");
        continue;
      }
      cmd_buffer = temp;
    }

    // Read arguments
    if (read(jms_in, cmd_buffer->args, cmd_buffer->len + 1) < 0) {
      perror("read");
      continue;
    }

    // Ensure null termination
    if (cmd_buffer->args[cmd_buffer->len] != '\0') {
      fprintf(stderr, "Received malformed arguments.\n");
      continue;
    }

    switch (cmd_buffer->action) {
    case SUBMIT:
      jobs_submit(cmd_buffer->args);
      break;
    case STATUS:
    case STATUS_ALL:
    case SHOW_ACTIVE:
    case SHOW_POOLS:
    case SHOW_FINISHED:
    case SUSPEND:
    case RESUME:
    case SHUTDOWN:
    default:
      fprintf(stderr, "Unknown command.\n");
      break;
    }
  }

  if (nread < 0) {
    if(errno == EAGAIN) {
      return 0;
    }
    return nread;
  }

  return 0;
}