#include "proc.h"

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/signalfd.h>
#include <unistd.h>
#include <wait.h>

#include "args.h"
#include "cmd.h"
#include "command.h"
#include "job.h"

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
  pipeout = fopen(str, "r+be");
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
  if (sigemptyset(&signals) < 0 || sigaddset(&signals, SIGCHLD) < 0 ||
      sigaddset(&signals, SIGTERM) < 0) {
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

static int exited = 0;

int proc_main(int id) {
  POOL_ID = id;

  if (proc_io_init() < 0) {
    return 0;
  }

  if (proc_sig_init() < 0) {
    proc_io_free();
    return 0;
  }

  if (job_init() < 0) {
    proc_io_free();
    proc_sig_free();
    return 0;
  }

  if (cmd_init() < 0) {
    proc_io_free();
    proc_sig_free();
    job_free();
    return 0;
  }

  struct pollfd fds[2];
  fds[0].fd = PIPEIN_FILENO;
  fds[0].events = POLLIN;
  fds[1].fd = SIG_FILENO;
  fds[1].events = POLLIN;

  int stop_received = 0;

  for (;;) {
    int ret = poll(fds, 2, -1);
    if (ret <= 0) {
      break;
    }

    if (fds[0].revents & POLLIN) {
      // Input data
      Command *cmd = cmd_read(PIPEIN_FILENO);
      if (cmd != NULL) {
        switch (cmd->action) {
        case SUBMIT:
          if (!stop_received) {
            job_add(cmd->data, cmd->args);
          }
          break;
        case STATUS:
          job_status(atoi(cmd->args));
          break;
        case STATUS_ALL: {
          int n = 0;
          if (cmd->len > 0) {
            n = atoi(cmd->args);
          }
          job_status_all(n);
        } break;
        case SHOW_ACTIVE:
          job_show_active();
          break;
        case SHOW_FINISHED:
          job_show_finished();
          break;
        case SUSPEND:
          if (!stop_received) {
            job_suspend(atoi(cmd->args));
          }
          break;
        case RESUME:
          if (!stop_received) {
            job_resume(atoi(cmd->args));
          }
          break;
        case SHUTDOWN:
        case SHOW_POOLS:
        case UNKNOWN:
          break;
        }
      }
    }

    if (fds[1].revents & POLLIN) {
      struct signalfd_siginfo siginfo;
      ssize_t nread =
          read(SIG_FILENO, &siginfo, sizeof(struct signalfd_siginfo));
      if (nread < 0) {
        continue;
      }

      if (siginfo.ssi_signo == SIGCHLD) {
        // wait(2)
        int wstatus;
        pid_t pid;
        while ((pid = waitpid(-1, &wstatus, WUNTRACED | WCONTINUED | WNOHANG)) >
               0) {
          if (WIFSTOPPED(wstatus)) {
            job_stopped(pid);
          } else if (WIFCONTINUED(wstatus)) {
            job_continued(pid);
          } else if (WIFEXITED(wstatus)) {
            job_exited(pid);
            exited++;
            if (exited == get_jobs_pool() ||
                (stop_received && exited == get_job_count())) {
              // ALL JOBS DONE
              goto stop;
            }
          }
        }
      } else if (siginfo.ssi_signo == SIGTERM) {
        // Graceful shutdown
        if (!stop_received) {
          stop_received = 1;
          if (job_shutdown() == 0) {
            goto stop;
          }
        }
      }
    }
  }

stop:
  cmd_free();
  job_free();
  proc_sig_free();
  proc_io_free();

  return 0;
}