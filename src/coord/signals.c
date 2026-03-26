#include "signals.h"

#include <signal.h>
#include <stdio.h>
#include <sys/signalfd.h>
#include <unistd.h>
#include <wait.h>

int signals_setup() {
  // Add SIGCHLD to be watched for
  // TODO: also handle SIGINT SIGQUIT
  // sigsetops(3)
  sigset_t signals;
  if (sigemptyset(&signals) < 0 || sigaddset(&signals, SIGCHLD) < 0) {
    return -1;
  }

  // Block default handling of signals
  // sigprocmask(2)
  if (sigprocmask(SIG_BLOCK, &signals, NULL) < 0) {
    return -1;
  }

  // signalfd(2)
  int signal_fd = signalfd(-1, &signals, SFD_CLOEXEC);
  if (signal_fd < 0) {
    return -1;
  }

  return signal_fd;
}

int signals_read(int signal_fd) {
  static struct signalfd_siginfo info;
  ssize_t nread;
  while ((nread = read(signal_fd, &info, sizeof(struct signalfd_siginfo))) >
         0) {
    if (info.ssi_signo == SIGCHLD) {
      // wait(2)
      int wstatus;
      if (waitpid(info.ssi_pid, &wstatus, WUNTRACED | WCONTINUED) < 0) {
        perror("waitpid");
        continue;
      }

      if (WIFSTOPPED(wstatus)) {
        // TODO: Child stopped
      } else if (WIFCONTINUED(wstatus)) {
        // TODO: Child continue
      } else if (WIFEXITED(wstatus)) {
        // TODO: Child exit
      }
    }
  }
  if (nread < 0) {
    return nread;
  }
  return 0;
}