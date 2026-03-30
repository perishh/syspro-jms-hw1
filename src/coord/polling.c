#include "polling.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_EVENTS 10

int EPOLL_FILENO;
struct epoll_event *_events;

int polling_init() {
  _events = malloc(sizeof(struct epoll_event) * MAX_EVENTS);
  if(_events == NULL) {
    return -1;
  }

  // epoll(7), epoll_create(2)
  EPOLL_FILENO = epoll_create1(EPOLL_CLOEXEC);
  if (EPOLL_FILENO < 0) {
    perror("epoll_create");
    free(_events);
    return -1;
  }

  return EPOLL_FILENO;
}

int polling_add(int fd) {
  struct epoll_event event;
  event.events = EPOLLIN;
  event.data.fd = fd;
  if (epoll_ctl(EPOLL_FILENO, EPOLL_CTL_ADD, fd, &event) < 0) {
    return -1;
  }

  return 0;
}

int polling_remove(int fd) {
  return epoll_ctl(EPOLL_FILENO, EPOLL_CTL_DEL, fd, NULL);
}

int polling_wait(struct epoll_event **events) {
  *events = _events;
  return epoll_wait(EPOLL_FILENO, _events, MAX_EVENTS, -1);
}

void polling_free() {
  close(EPOLL_FILENO);
  free(_events);
}