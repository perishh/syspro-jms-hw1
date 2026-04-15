#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "args.h"
#include "cmd.h"
#include "command.h"
#include "io.h"
#include "polling.h"
#include "pool.h"
#include "sig.h"
#include "utils.h"

int main(int argc, char **argv) {
  args_init(argc, argv);
  cmd_init();
  polling_init();
  io_init();
  sig_init();
  pool_init();

  struct epoll_event *events;
  for (;;) {
    int count = polling_wait(&events);
    if (count < 0) {
      // TODO
    }

    for (int i = 0; i < count; i++) {
      if (events[i].data.fd == JMSIN_FILENO) {
        Command *cmd = cmd_read(JMSIN_FILENO);
        if (cmd == NULL) {
          continue;
        }

        switch (cmd->action) {
        case SUBMIT:
          pool_submit(cmd);
          break;
        case SUSPEND:
        case RESUME:
        case STATUS_ALL:
        case SHOW_ACTIVE:
        case STATUS:
          pool_broadcast(cmd);
          break;
        case SHOW_FINISHED:
          pool_broadcast(cmd);
          // TODO: Show finished pools' jobs
          break;
        case SHOW_POOLS:
          pool_show();
          break;
        case SHUTDOWN:
        // TODO
        case UNKNOWN:
          break;
        }
      } else if (events[i].data.fd == SIG_FILENO) {
        SignalInfo sig;
        if (decode_signal(SIG_FILENO, &sig) < 0) {
          continue;
        }
        // TODO
      } else {
        // Input from pool process
        pool_redirect(events[i].data.fd);
      }
    }
  }

  pool_free();
  cmd_free();
  polling_free();
  io_free();
  sig_free();
}