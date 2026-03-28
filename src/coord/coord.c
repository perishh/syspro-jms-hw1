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

  // epoll(7), epoll_create(2)
  int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  if (epoll_fd < 0) {
    perror("epoll_create");
    return 1;
  }

  int jms_in = pipes_setup(epoll_fd);
  if (jms_in < 0) {
    perror("pipes setup");

    close(epoll_fd);
    return 1;
  }

  Command *cmd_buffer = malloc(sizeof(Command));
  if (cmd_buffer == NULL) {
    perror("malloc");

    close(epoll_fd);
    pipes_free();
    return 1;
  }

  int signal_fd = signals_setup(epoll_fd);
  if (signal_fd < 0) {
    perror("setup signals");

    free(cmd_buffer);
    close(epoll_fd);
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
      pipes_free();
      return 1;
    }
    for (int i = 0; i < count; i++) {
      if (events[i].data.fd == signal_fd) {
        // Signal received
        if (signals_read(signal_fd) < 0) {
          perror("signal_fd");
          continue;
        }
      } else if (events[i].data.fd == jms_in) {
        // Command received
        if (parse_commands(jms_in, cmd_buffer) < 0) {
          perror("parse_commands");
          continue;
        }
      }
    }
  }

  free(cmd_buffer);
  close(epoll_fd);
  close(signal_fd);
  pipes_free();

  return 0;
}