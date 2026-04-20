#include "io.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "globals.h"
#include "polling.h"

#define JMS_IN "jms_in"
#define JMS_OUT "jms_out"

int JMSIN_FILENO;
int JMSOUT_FILENO;

int io_init() {
  unlink(JMS_IN);
  unlink(JMS_OUT);

  if (mkfifo(JMS_IN, MODE_RW) < 0) {
    return -1;
  }

  if (mkfifo(JMS_OUT, MODE_RW) < 0) {
    unlink(JMS_IN);
    return -1;
  }

  JMSIN_FILENO = open(JMS_IN, O_RDWR | O_NONBLOCK | O_CLOEXEC);
  if (JMSIN_FILENO < 0) {
    unlink(JMS_IN);
    unlink(JMS_OUT);
    return -1;
  }

  polling_add(JMSIN_FILENO);

  JMSOUT_FILENO = open(JMS_OUT, O_RDWR | O_NONBLOCK | O_CLOEXEC);
  if (JMSOUT_FILENO < 0) {
    close(JMSIN_FILENO);
    unlink(JMS_IN);
    unlink(JMS_OUT);
    return -1;
  }

  return 0;
}

void io_close() {
  close(JMSIN_FILENO);
  close(JMSOUT_FILENO);
}

void io_free() {
  unlink(JMS_IN);
  unlink(JMS_OUT);
}