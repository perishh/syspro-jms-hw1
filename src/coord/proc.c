#include "proc.h"

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/signalfd.h>
#include <unistd.h>

#include "cmd.h"
#include "command.h"
#include "job.h"
#include "utils.h"

int POOL_ID;
int PIPEIN_FILENO;
FILE *pipeout;

int proc_io_init() {
  char str[32];
  sprintf(str, "pool_%d_in", POOL_ID);

  PIPEIN_FILENO = open(str, O_RDWR | O_NONBLOCK | O_CLOEXEC);
  if (PIPEIN_FILENO < 0) {
    return -1;
  }

  sprintf(str, "pool_%d_out", POOL_ID);
  pipeout = fopen(str, "r+e");
  if (pipeout == NULL) {
    close(PIPEIN_FILENO);
    return -1;
  }

  return 0;
}

void proc_io_free() {
  close(PIPEIN_FILENO);
  fclose(pipeout);
}

static int SIG_FILENO;
int proc_sig_init() {
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
  SIG_FILENO = signalfd(-1, &signals, SFD_CLOEXEC | SFD_NONBLOCK);
  if (SIG_FILENO < 0) {
    return -1;
  }

  return 0;
}

void proc_sig_free() { close(SIG_FILENO); }

int proc_main(int id) {
  POOL_ID = id;
  proc_io_init();
  proc_sig_init();
  job_init();
  cmd_init(); // TODO: Check if causes issues

  struct pollfd fds[2];
  fds[0].fd = PIPEIN_FILENO;
  fds[0].events = POLLIN;
  fds[1].fd = SIG_FILENO;
  fds[1].events = POLLIN;

  for (;;) {
    int ret = poll(fds, 2, -1);
    if (ret <= 0) {
      // TODO
      // continue;
    }

    if (fds[0].revents & POLLIN) {
      // Input data
      Command *cmd = cmd_read(PIPEIN_FILENO);
      if (cmd != NULL) {
        switch (cmd->action) {
        case SUBMIT:
          job_add(cmd->data, cmd->args);
          break;
        case STATUS:
          job_status(atoi(cmd->args));
          break;
        case STATUS_ALL:
          job_status(atoi(cmd->args));
          break;
        case SHOW_ACTIVE:
          job_show_active();
          break;
        case SHOW_FINISHED:
          job_show_finished();
          break;
        case SUSPEND:
          job_status(atoi(cmd->args));
          break;
        case RESUME:
          job_status(atoi(cmd->args));
          break;
        case SHUTDOWN:
        case SHOW_POOLS:
        case UNKNOWN:
          break;
        }
      }
    }

    if (fds[1].revents & POLLIN) {
      SignalInfo sig;
      if (decode_signal(SIG_FILENO, &sig) >= 0) {
        switch (sig.cause) {
        case STOPPED:
          job_stopped(sig.pid);
          break;
        case CONTINUED:
          job_continued(sig.pid);
          break;
        case EXITED:
          job_exited(sig.pid);
          break;
        }
      }
    }
  }

  cmd_free();
  job_free();
  proc_sig_free();
  proc_io_free();

  return 0;
}