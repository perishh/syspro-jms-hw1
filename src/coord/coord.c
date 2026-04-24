#include <signal.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/wait.h>
#include <unistd.h>

#include "args.h"
#include "cmd.h"
#include "command.h"
#include "io.h"
#include "polling.h"
#include "pool.h"
#include "sig.h"

int main(int argc, char **argv) {
  if (args_init(argc, argv) < 0) {
    return 1;
  }

  if (cmd_init() < 0) {
    return 1;
  }

  if (polling_init() < 0) {
    cmd_free();
    return 1;
  }

  if (io_init() < 0) {
    cmd_free();
    polling_free();
    return 1;
  }

  if (sig_init() < 0) {
    cmd_free();
    polling_free();
    io_close();
    io_free();
    return 1;
  }

  if (pool_init() < 0) {
    cmd_free();
    polling_free();
    io_close();
    io_free();
    sig_free();
    return 1;
  }

  int shutting_down = 0;
  int status = 0;

  struct epoll_event *events;
  for (;;) {
    int count = polling_wait(&events);
    if (count < 0) {
      status = 1;
      break;
    }

    for (int i = 0; i < count; i++) {
      if (events[i].data.fd == JMSIN_FILENO) {
        Command *cmd = cmd_read(JMSIN_FILENO);
        if (cmd == NULL) {
          continue;
        }

        switch (cmd->action) {
        case SUBMIT:
          if (!shutting_down) {
            pool_submit(cmd);
          }
          break;
        case SUSPEND:
        case RESUME:
          if (!shutting_down) {
            pool_broadcast(cmd);
          }
          break;
        case STATUS_ALL:
          pool_status_all(cmd);
          break;
        case STATUS:
          pool_status(cmd);
          break;
        case SHOW_ACTIVE:
          pool_broadcast(cmd);
          break;
        case SHOW_FINISHED:
          pool_finished();
          break;
        case SHOW_POOLS:
          pool_show();
          break;
        case SHUTDOWN:
          shutting_down = 1;
          if (pool_shutdown()) {
            // NO POOLS ACTIVE
            goto stop;
          }
          break;
        case UNKNOWN:
          break;
        }
      } else if (events[i].data.fd == SIG_FILENO) {
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
          while ((pid = waitpid(-1, &wstatus,
                                WUNTRACED | WCONTINUED | WNOHANG)) > 0) {
            if (WIFEXITED(wstatus)) {
              // Pool exited
              if (pool_exited(pid) && shutting_down) {
                // ALL POOLS EXITED & SHUTDOWN RECEIVED
                goto stop;
              }
            }
          }
        } else {
          shutting_down = 1;
          if (pool_shutdown()) {
            // NO POOLS ACTIVE
            goto stop;
          }
        }
      } else {
        // Input from pool process
        pool_redirect(events[i].data.fd);
      }
    }
  }

stop:

  pool_print_info();

  {
    char eot = 0x04;
    write(JMSOUT_FILENO, &eot, 1);
  }

  pool_free();
  cmd_free();
  polling_free();
  io_close();
  io_free();
  sig_free();

  return status;
}