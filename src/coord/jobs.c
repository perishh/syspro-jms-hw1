#include "jobs.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>

#include "globals.h"
#include "list.h"

// TODO: Check if I can make it a separate program
// and run it with exec

int key;
int active = 0;

typedef struct {
  int id;
  pid_t pid; // 0 indicates empty slot
  int running;
  time_t timestamp;
} Job;

LinkedList jobs;
LinkedList finished;

int epoll_fd;

void jobs_add(int argc, char **argv) {}

void jobs_init(int id) {
  key = (id - 1) * jobs_pool;

  ll_init(&jobs);
  ll_init(&finished);

  char str[32];
  sprintf(str, "pool_%d_in", id);

  int in = open(str, O_RDONLY | O_CLOEXEC | O_NONBLOCK);
  if (in < 0) {
    ll_free(&jobs);
    ll_free(&finished);
    exit(1);
  }

  if (dup2(in, STDIN_FILENO) < 0) {
    close(in);
    ll_free(&jobs);
    ll_free(&finished);
    exit(1);
  }

  sprintf(str, "pool_%d_out", id);

  int out = open(str, O_WRONLY | O_CLOEXEC | O_NONBLOCK);
  if (out < 0) {
    close(in);
    ll_free(&jobs);
    ll_free(&finished);
    exit(1);
  }

  if (dup2(out, STDOUT_FILENO) < 0) {
    close(in);
    close(out);
    ll_free(&jobs);
    ll_free(&finished);
    exit(1);
  }

  int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  if (epoll_fd < 0) {
    close(in);
    close(out);
    ll_free(&jobs);
    ll_free(&finished);
    exit(1);
  }

  struct epoll_event event;
  event.events = EPOLLIN;
  event.data.fd = STDIN_FILENO;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, STDIN_FILENO, &event) < 0) {
    close(in);
    close(out);
    close(epoll_fd);
    ll_free(&jobs);
    ll_free(&finished);
    exit(1);
  }

  int max_events = jobs_pool + 1;
  struct epoll_event *events =
      malloc(sizeof(struct epoll_event) * (max_events));
  if (events == NULL) {
    close(in);
    close(out);
    close(epoll_fd);
    ll_free(&jobs);
    ll_free(&finished);
    exit(1);
  }

  for (;;) {
    int count = epoll_wait(epoll_fd, events, max_events, -1);
    if (count < 0) {
      // TODO
      break;
    }
    for (int i = 0; i < count; i++) {
      if (events[i].data.fd == STDIN_FILENO) {
        // Command from coord
        // TODO
      }
    }
  }

  free(events);
  close(in);
  close(out);
  close(epoll_fd);
  ll_free(&jobs);
  ll_free(&finished);

  exit(0);
}