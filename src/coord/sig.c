#include "sig.h"
#include "polling.h"

#include <signal.h>
#include <sys/signalfd.h>
#include <sys/wait.h>
#include <unistd.h>

int SIG_FILENO;

int sig_init() {
  // sigsetops(3)
  sigset_t signals;
  if (sigemptyset(&signals) < 0 || sigaddset(&signals, SIGCHLD) < 0 ||
      sigaddset(&signals, SIGINT) < 0 || sigaddset(&signals, SIGTERM) < 0) {
    return -1;
  }

  // Block default handling of signals
  // sigprocmask(2)
  if (sigprocmask(SIG_BLOCK, &signals, NULL) < 0) {
    return -1;
  }

  // signalfd(2)
  SIG_FILENO = signalfd(-1, &signals, SFD_CLOEXEC | SFD_NONBLOCK);
  if (SIG_FILENO < 0) {
    return -1;
  }

  if (polling_add(SIG_FILENO) < 0) {
    return -1;
  }

  return 0;
}

void sig_free() { close(SIG_FILENO); }