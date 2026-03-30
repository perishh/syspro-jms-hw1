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
#include "pools.h"
#include "signals.h"

#define MAX_EVENTS 10

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

  // epoll(7), epoll_create(2)
  int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  if (epoll_fd < 0) {
    perror("epoll_create");
    pools_free();
    return 1;
  }

  int jms_in = pipes_setup(epoll_fd);
  if (jms_in < 0) {
    perror("pipes setup");

    pools_free();
    close(epoll_fd);
    return 1;
  }

  int buffer_size = sizeof(Command);
  Command *cmd_buffer = malloc(buffer_size);
  if (cmd_buffer == NULL) {
    perror("malloc");

    close(epoll_fd);
    pools_free();
    pipes_free();
    return 1;
  }

  SignalInfo signal;
  int signal_fd = signals_setup(epoll_fd);
  if (signal_fd < 0) {
    perror("setup signals");

    free(cmd_buffer);
    close(epoll_fd);
    pools_free();
    pipes_free();
    return 1;
  }

  struct epoll_event events[MAX_EVENTS];
  for (;;) {
    int count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (count < 0) {
      close(epoll_fd);
      close(signal_fd);
      free(cmd_buffer);
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
        printf("There is data in input fifo\n");
        // Command received
        if (parse_commands(jms_in, &cmd_buffer, &buffer_size) < 0) {
          continue;
        }
        printf("Action: %d %s\n", cmd_buffer->action, cmd_buffer->args);
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

  free(cmd_buffer);
  close(epoll_fd);
  close(signal_fd);
  pipes_free();
  pools_free();

  return 0;
}