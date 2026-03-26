#include "pipes.h"

#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "globals.h"

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
  int jms_in = open(JMS_IN, O_RDONLY | O_CLOEXEC | O_NONBLOCK);
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

  return jms_in;
}