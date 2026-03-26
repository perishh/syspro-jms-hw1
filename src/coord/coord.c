#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "command.h"
#include "globals.h"
#include "jobs.h"

#define JMS_IN "jms_in"
#define JMS_OUT "jms_out"

int jobs_pool = 0;

void print_usage() {
  fprintf(stderr, "Usage: jms_coord -l <path> -n <jobs_pool>\n");
}

int main(int argc, char **argv) {
  char *path = NULL;

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

  // open(2)
  int jms_in = open(JMS_IN, O_RDONLY | O_CLOEXEC);
  if (jms_in < 0) {
    perror("open (jms_in)");

    unlink(JMS_IN);
    unlink(JMS_OUT);
    return 1;
  }

  ssize_t buffer_size = sizeof(Command);
  Command *cmd = malloc(buffer_size);
  if (cmd == NULL) {
    perror("malloc");
    close(jms_in);
    unlink(JMS_IN);
    unlink(JMS_OUT);
    return 1;
  }

  // read(2)
  ssize_t nread;
  while ((nread = read(jms_in, cmd, sizeof(Command))) > 0) {
    if ((cmd->action & ZERO_ARG_ACTIONS) != 0) {
      // Command doesn't require args
      // TODO: Implement
      continue;
    }

    if (cmd->len <= 0) {
      // TODO: Throw error, invalid argument size
    }

    // Check current buffer size and expand if needed
    ssize_t required_space = sizeof(Command) + cmd->len + 1;
    if (buffer_size < required_space) {
      buffer_size = required_space;

      Command *temp = cmd;
      cmd = realloc(cmd, required_space);
      if (cmd == NULL) {
        perror("realloc");

        free(temp);
        close(jms_in);
        unlink(JMS_IN);
        unlink(JMS_OUT);
        return 1;
      }
    }

    // Read arguments
    if (read(jms_in, cmd->args, cmd->len + 1) < 0) {
      perror("read (args)");

      free(cmd);
      close(jms_in);
      unlink(JMS_IN);
      unlink(JMS_OUT);
      return 1;
    }

    // Ensure null termination
    if (cmd->args[cmd->len] != '\0') {
      fprintf(stderr, "Received malformed arguments.\n");
      continue;
    }

    switch (cmd->action) {
    case SUBMIT:
      jobs_submit(cmd->args);
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
    perror("read (cmd)");

    close(jms_in);
    free(cmd);
    unlink(JMS_IN);
    unlink(JMS_OUT);
    return 1;
  }

  close(jms_in);
  free(cmd);
  unlink(JMS_IN);
  unlink(JMS_OUT);

  return 0;
}