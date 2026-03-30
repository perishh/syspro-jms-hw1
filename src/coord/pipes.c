#include "pipes.h"

#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "polling.h"
#include "globals.h"

int jms_in;
int JMSOUT_FILENO;

int pipes_setup() {
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
  
  if (polling_add(jms_in) < 0) {
    close(jms_in);
    unlink(JMS_IN);
    unlink(JMS_OUT);
    return -1;
  }

  // Open as read-write to avoid blocking if no reader
  JMSOUT_FILENO = open(JMS_OUT, O_RDWR | O_NONBLOCK | O_CLOEXEC);
  if (JMSOUT_FILENO < 0) {
    polling_remove(jms_in);
    close(jms_in);
    unlink(JMS_IN);
    unlink(JMS_OUT);
    return -1;
  }

  return jms_in;
}

void pipes_close() {
  close(JMSOUT_FILENO);
  close(jms_in);
}

void pipes_free() {
  pipes_close();
  unlink(JMS_IN);
  unlink(JMS_OUT);
}