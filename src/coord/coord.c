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
#include "signals.h"

#define JMS_IN "jms_in"
#define JMS_OUT "jms_out"

#define MAX_EVENTS 10

int jobs_pool = 0;

Command *cmd_buffer = NULL;

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

  cmd_buffer = malloc(sizeof(Command));
  if (cmd_buffer == NULL) {
    perror("malloc");
    close(jms_in);
    unlink(JMS_IN);
    unlink(JMS_OUT);
    return 1;
  }

  int signal_fd = signals_setup();
  if (signal_fd < 0) {
    perror("setup signals");
    free(cmd_buffer);
    close(jms_in);
    unlink(JMS_IN);
    unlink(JMS_OUT);
    return 1;
  }

  // epoll(7), epoll_create(2)
  int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  if (epoll_fd < 0) {
    close(jms_in);
    close(signal_fd);
    free(cmd_buffer);
    unlink(JMS_IN);
    unlink(JMS_OUT);
    return 1;
  }

  struct epoll_event event;

  event.events = EPOLLIN;
  event.data.fd = signal_fd;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, signal_fd, &event) < 0) {
    close(epoll_fd);
    close(jms_in);
    close(signal_fd);
    free(cmd_buffer);
    unlink(JMS_IN);
    unlink(JMS_OUT);
    return 1;
  }

  event.events = EPOLLIN;
  event.data.fd = jms_in;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, jms_in, &event) < 0) {
    close(epoll_fd);
    close(jms_in);
    close(signal_fd);
    free(cmd_buffer);
    unlink(JMS_IN);
    unlink(JMS_OUT);
    return 1;
  }

  struct epoll_event events[MAX_EVENTS];
  for (;;) {
    int count = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (count < 0) {
      close(epoll_fd);
      close(jms_in);
      close(signal_fd);
      free(cmd_buffer);
      unlink(JMS_IN);
      unlink(JMS_OUT);
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
          perror("read_commands");
          continue;
        }
      }
    }
  }

  close(epoll_fd);
  close(jms_in);
  close(signal_fd);
  free(cmd_buffer);
  unlink(JMS_IN);
  unlink(JMS_OUT);

  return 0;
}