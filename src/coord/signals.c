#include "signals.h"

#include <signal.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <unistd.h>
#include <wait.h>

int signals_setup(int epoll_fd) {
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
  int signal_fd = signalfd(-1, &signals, SFD_CLOEXEC | SFD_NONBLOCK);
  if (signal_fd < 0) {
    return -1;
  }

  struct epoll_event event;

  event.events = EPOLLIN;
  event.data.fd = signal_fd;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, signal_fd, &event) < 0) {
    return -1;
  }

  return signal_fd;
}

int signals_read(int signal_fd, SignalInfo *info) {
  static struct signalfd_siginfo siginfo;

  ssize_t nread = read(signal_fd, &siginfo, sizeof(struct signalfd_siginfo));
  if (nread < 0) {
    return -1;
  }

  if (siginfo.ssi_signo == SIGCHLD) {
    // wait(2)
    int wstatus;
    if (waitpid(siginfo.ssi_pid, &wstatus, WUNTRACED | WCONTINUED) < 0) {
      return -1;
    }

    info->pid = siginfo.ssi_pid;
    if (WIFSTOPPED(wstatus)) {
      info->cause = STOPPED;
    } else if (WIFCONTINUED(wstatus)) {
      info->cause = CONTINUED;
    } else if (WIFEXITED(wstatus)) {
      info->cause = EXITED;
    } else {
      return -1;
    }
  } else {
    return -1;
  }

  return 0;
}