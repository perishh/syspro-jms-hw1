#include "parser.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
    if (cmd_buffer->len == 0) {
      // No arguments given
      switch (cmd_buffer->action) {
      case STATUS_ALL:
        jobs_status_all(INT_MAX);
        break;
      case SHOW_ACTIVE:
        jobs_show_active();
        break;
      case SHOW_FINISHED:
        jobs_show_finished();
        break;
      case SHOW_POOLS:
      case SHUTDOWN:
      default:
        printf("Invalid command\n");
      }
      continue;
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
      printf("Malformed arguments\n");
      continue;
    }

    switch (cmd_buffer->action) {
    case SUBMIT:
      jobs_submit(cmd_buffer->args);
      break;
    case SUSPEND:
      jobs_suspend(atoi(cmd_buffer->args));
      break;
    case RESUME:
      jobs_resume(atoi(cmd_buffer->args));
      break;
    case STATUS_ALL:
      jobs_status_all(atoi(cmd_buffer->args));
      break;
    case STATUS:
      jobs_status(atoi(cmd_buffer->args));
      break;
    default:
      printf("Invalid command\n");
      break;
    }
  }

  if (nread < 0) {
    if (errno == EAGAIN) {
      return 0;
    }
    return nread;
  }

  return 0;
}