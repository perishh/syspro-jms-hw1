#include "pipes.h"

#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "globals.h"

int jms_in;
int jms_out;

int pipes_setup(int epoll_fd) {
  // Clear leftover files
  unlink(JMS_IN);
  unlink(JMS_OUT);

  // Create fifos
  // mkfifo(3)
  if (mkfifo(JMS_IN, MODE_RW) < 0) {
    return -1;
  }

  if (mkfifo(JMS_OUT, MODE_RW) < 0) {
    unlink(JMS_IN);
    return -1;
  }

  // open(2)
  // Open as read-write to avoid getting EOF when console exits
  jms_in = open(JMS_IN, O_RDWR | O_CLOEXEC | O_NONBLOCK);
  if (jms_in < 0) {
    unlink(JMS_IN);
    unlink(JMS_OUT);
    return jms_in;
  }

  struct epoll_event event;
  event.events = EPOLLIN;
  event.data.fd = jms_in;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, jms_in, &event) < 0) {
    close(jms_in);
    unlink(JMS_IN);
    unlink(JMS_OUT);
    return -1;
  }

  // Open as read-write to avoid blocking if no reader
  jms_out = open(JMS_OUT, O_RDWR | O_NONBLOCK | O_CLOEXEC);
  if (jms_out < 0) {
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, jms_in, &event);
    close(jms_in);
    unlink(JMS_IN);
    unlink(JMS_OUT);
    return -1;
  }

  // TODO: Handle closing
  // if (dup2(jms_out, STDOUT_FILENO) < 0) {
  //   close(jms_out);
  //   epoll_ctl(epoll_fd, EPOLL_CTL_DEL, jms_in, &event);
  //   close(jms_in);
  //   unlink(JMS_IN);
  //   unlink(JMS_OUT);
  //   return -1;
  // }

  return jms_in;
}

void pipes_free() {
  close(jms_out);
  close(jms_in);
  unlink(JMS_IN);
  unlink(JMS_OUT);
}