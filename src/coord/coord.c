#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "command.h"
#include "globals.h"
#include "parser.h"
#include "pipes.h"
#include "polling.h"
#include "pools.h"
#include "signals.h"

int jobs_pool = 0;

int main(int argc, char **argv) {
  char *path = NULL;

  if (parse_arguments(argc, argv, &path, &jobs_pool) < 0) {
    return 1;
  }

  // Change working directory to path
  // chdir(2)
  if (chdir(path) < 0) {
    perror("chdir");
    return 1;
  }

  if (pools_init() < 0) {
    return 1;
  }

  if (polling_init() < 0) {
    perror("epoll_init");
    pools_free();
    return 1;
  }

  int jms_in = pipes_setup();
  if (jms_in < 0) {
    perror("pipes setup");

    pools_free();
    polling_free();
    return 1;
  }

  int buffer_size = sizeof(Command);
  Command *cmd_buffer = malloc(buffer_size);
  if (cmd_buffer == NULL) {
    perror("malloc");

    polling_free();
    pools_free();
    pipes_free();
    return 1;
  }

  SignalInfo signal;
  int signal_fd = signals_setup();
  if (signal_fd < 0) {
    perror("setup signals");

    free(cmd_buffer);
    polling_free();
    pools_free();
    pipes_free();
    return 1;
  }
  
  struct epoll_event *events;
  for (;;) {
    int count = polling_wait(&events);
    if (count < 0) {
      close(signal_fd);
      free(cmd_buffer);
      polling_free();
      pools_free();
      pipes_free();
      return 1;
    }
    for (int i = 0; i < count; i++) {
      if (events[i].data.fd == signal_fd) {
        // Signal received
        if (signals_read(signal_fd, &signal) < 0) {
          continue;
        }
        // TODO: Handle
      } else if (events[i].data.fd == jms_in) {
        // Command received
        if (parse_commands(jms_in, &cmd_buffer, &buffer_size) < 0) {
          continue;
        }
        switch (cmd_buffer->action) {
        case SUBMIT:
          pools_enqueue(cmd_buffer->len, cmd_buffer->args);
          break;
        case STATUS:
        case STATUS_ALL:
        case SHOW_ACTIVE:
        case SHOW_POOLS:
        case SHOW_FINISHED:
        case SUSPEND:
        case RESUME:
        case SHUTDOWN:
        case UNKNOWN:
          break;
        }
        // TODO: Handle
      }
    }
  }

  polling_free();
  pipes_free();
  pools_free();
  close(signal_fd);
  free(cmd_buffer);

  return 0;
}