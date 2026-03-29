#include "jobs.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "globals.h"
#include "list.h"
#include "outputs.h"
#include "parser.h"
#include "signals.h"
#include "utils.h"

// TODO: Check if I can make it a separate program
// and run it with exec

int PIPEIN;
FILE *PIPEOUT;

int init_pipes(int id, int epoll_fd);
void close_pipes();

int active = 0;

typedef struct {
  int id;
  pid_t pid; // 0 indicates empty slot
  int running;
  time_t timestamp;
} Job;

LinkedList jobs;
LinkedList finished;

// int epoll_fd;

int jobs_add(int id, char *raw) {
  if (active >= jobs_pool) {
    return -1;
  }

  int argc = count_words(raw);
  char *argv[argc + 1]; // Account for terminating NULL; TODO: Consider malloc
  if (decode_args(raw, argv) < 0) {
    return -1;
  }

  Job *job = malloc(sizeof(Job));
  if (job == NULL) {
    return -1;
  }

  job->id = id;
  job->running = 1;
  // time(2)
  job->timestamp = time(NULL);

  // fork(2)
  job->pid = fork();
  if (job->pid == 0) {
    // Job process

    // Change working directory
    if (outputs_cd(job->id, job->timestamp) < 0) {
      return 1;
    }

    // Redirect stdout & stderr to files
    if (outputs_redirect(job->id) < 0) {
      return 1;
    }

    // exec(3)
    int ret = execvp(argv[0], argv);
    if (ret < 0) {
      return ret;
    }
  }

  printf("Forked\n");

  if (job->pid < 0) {
    free(job);
    return -1;
  }

  if (ll_push(&jobs, job) < 0) {
    return -1;
  }
  
  fprintf(PIPEOUT, "JobID: %d, PID: %d\n", job->id, job->pid);
  fflush(PIPEOUT);
  printf("Returned\n");
  return 0;
}

void jobs_init(int id) {
  // Pool process entry point
  printf("Start\n");
  ll_init(&jobs);
  ll_init(&finished);

  int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  if (epoll_fd < 0) {
    ll_free(&jobs);
    ll_free(&finished);
    exit(1);
  }

  if (init_pipes(id, epoll_fd) < 0) {
    close(epoll_fd);
    ll_free(&jobs);
    ll_free(&finished);
    exit(1);
  }

  Command *cmd_buffer = malloc(sizeof(Command));
  if (cmd_buffer == NULL) {
    close_pipes();
    close(epoll_fd);
    ll_free(&jobs);
    ll_free(&finished);
    exit(1);
  }

  SignalInfo signal;
  int signal_fd = signals_setup(epoll_fd);
  if (signal_fd < 0) {
    close_pipes();
    close(epoll_fd);
    ll_free(&jobs);
    ll_free(&finished);
    exit(1);
  }

  int max_events = jobs_pool + 1;
  struct epoll_event *events =
      malloc(sizeof(struct epoll_event) * (max_events));
  if (events == NULL) {
    close_pipes();
    close(epoll_fd);
    ll_free(&jobs);
    ll_free(&finished);
    exit(1);
  }
  printf("Started\n");
  for (;;) {
    int count = epoll_wait(epoll_fd, events, max_events, -1);
    if (count < 0) {
      // TODO
      break;
    }
    for (int i = 0; i < count; i++) {
      if (events[i].data.fd == PIPEIN) {
        if (parse_commands(PIPEIN, &cmd_buffer) < 0) {
          continue;
        }
        switch (cmd_buffer->action) {
        case SUBMIT:
          printf("Adding job\n");
          jobs_add(cmd_buffer->data, cmd_buffer->args);
          break;
        case STATUS:
        case STATUS_ALL:
        case SHOW_ACTIVE:
        case SHOW_POOLS:
        case SHOW_FINISHED:
        case SUSPEND:
        case RESUME:
        case SHUTDOWN:
        case UNKNOWN:
          break;
        }
        // TODO
      } else if (events[i].data.fd == signal_fd) {
        if (signals_read(signal_fd, &signal) < 0) {
          continue;
        }
        // TODO
      }
    }
  }

  free(events);
  close(signal_fd);
  close(epoll_fd);
  ll_free(&jobs);
  ll_free(&finished);

  close_pipes();

  exit(0);
}

int init_pipes(int id, int epoll_fd) {
  char str[32];
  sprintf(str, "pool_%d_in", id);

  PIPEIN = open(str, O_RDONLY | O_CLOEXEC | O_NONBLOCK);
  if (PIPEIN < 0) {
    return -1;
  }

  sprintf(str, "pool_%d_out", id);

  PIPEOUT = fopen(str, "r+e");
  if (PIPEOUT == NULL) {
    close(PIPEIN);
    return -1;
  }

  struct epoll_event event;
  event.events = EPOLLIN;
  event.data.fd = PIPEIN;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, PIPEIN, &event) < 0) {
    close(PIPEIN);
    fclose(PIPEOUT);
    return -1;
  }

  printf("Added to epoll\n");

  return 0;
}
void close_pipes() {
  close(PIPEIN);
  fclose(PIPEOUT);
}