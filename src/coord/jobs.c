#include "jobs.h"

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/signalfd.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "globals.h"
#include "outputs.h"
#include "parser.h"
#include "signals.h"
#include "utils.h"

// TODO: Check if I can make it a separate program
// and run it with exec

int PIPEIN;
FILE *PIPEOUT;

#define SENDF(fmt, ...)                                                        \
  {                                                                            \
    fprintf(PIPEOUT, fmt __VA_OPT__(, ) __VA_ARGS__);                          \
    fflush(PIPEOUT);                                                           \
  }

int init_pipes(int id);
void close_pipes();
int setup_signals();

typedef struct {
  int id;
  pid_t pid; // 0 indicates empty slot
  int suspended;
  int finished;
  time_t timestamp;
} Job;

int size = 0;
int finished = 0;
Job *jobs;

int jobs_add(int id, char *raw) {
  if (size >= jobs_pool) {
    return -1;
  }

  int argc = count_words(raw);
  char *argv[argc + 1]; // Account for terminating NULL; TODO: Consider malloc
  if (decode_args(raw, argv) < 0) {
    return -1;
  }

  Job *job = &jobs[size++];

  job->id = id;
  job->suspended = 0;
  job->finished = 0;
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

  if (job->pid < 0) {
    size--;
    return -1;
  }

  SENDF("JobID: %d, PID: %d\n", job->id, job->pid);
  return 0;
}

int jobs_suspend(int id) {
  Job *job = NULL;
  for (int i = 0; i < size; i++) {
    if (jobs[i].id == id) {
      job = &jobs[i];
      break;
    }
  }

  if (job == NULL || job->finished || job->suspended) {
    return -1;
  }

  // kill(2)
  if (kill(job->pid, SIGSTOP) < 0) {
    return -1;
  }

  SENDF("Sent suspend signal to JobID %d\n", id);
  return 0;
}

int jobs_resume(int id) {
  Job *job = NULL;
  for (int i = 0; i < size; i++) {
    if (jobs[i].id == id) {
      job = &jobs[i];
      break;
    }
  }

  if (job == NULL || job->finished || !job->suspended) {
    return -1;
  }

  // kill(2)
  if (kill(job->pid, SIGCONT) < 0) {
    return -1;
  }

  SENDF("Sent resume signal to JobID %d\n", id);
  return 0;
}

int jobs_continued(pid_t pid) {
  Job *job = NULL;
  for (int i = 0; i < size; i++) {
    if (jobs[i].pid == pid) {
      job = &jobs[i];
      break;
    }
  }

  if (job == NULL) {
    return -1;
  }

  job->suspended = 0;
  return 0;
}

int jobs_stopped(pid_t pid) {
  Job *job = NULL;
  for (int i = 0; i < size; i++) {
    if (jobs[i].pid == pid) {
      job = &jobs[i];
      break;
    }
  }

  if (job == NULL) {
    return -1;
  }

  job->suspended = 1;
  return 0;
}

int jobs_exited(pid_t pid) {
  Job *job = NULL;
  for (int i = 0; i < size; i++) {
    if (jobs[i].pid == pid) {
      job = &jobs[i];
      break;
    }
  }

  if (job == NULL) {
    return -1;
  }

  job->finished = 1;
  return 0;
}

void jobs_init(int id) {
  // Pool process entry point

  if (init_pipes(id) < 0) {
    exit(1);
  }

  jobs = malloc(sizeof(Job) * jobs_pool);
  if (jobs == NULL) {
    close_pipes();
    exit(1);
  }

  int buffer_size = sizeof(Command);
  Command *cmd_buffer = malloc(buffer_size);
  if (cmd_buffer == NULL) {
    close_pipes();
    free(jobs);
    exit(1);
  }

  SignalInfo signal;
  int signal_fd = setup_signals();
  if (signal_fd < 0) {
    close_pipes();
    free(jobs);
    free(cmd_buffer);
    exit(1);
  }

  struct pollfd fds[2];
  fds[0].fd = PIPEIN;
  fds[0].events = POLLIN;

  fds[1].fd = signal_fd;
  fds[1].events = POLLIN;

  for (;;) {
    int ret = poll(fds, 2, -1);
    if (ret <= 0) {
      // TODO
      break;
    }

    if (fds[0].revents & POLLIN) {
      if (parse_commands(PIPEIN, &cmd_buffer, &buffer_size) >= 0) {
        switch (cmd_buffer->action) {
        case SUBMIT:
          jobs_add(cmd_buffer->data, cmd_buffer->args);
          break;
        case SUSPEND:
          jobs_suspend(atoi(cmd_buffer->args));
          break;
        case RESUME:
          jobs_resume(atoi(cmd_buffer->args));
          break;
        case STATUS:
        case STATUS_ALL:
        case SHOW_ACTIVE:
        case SHOW_POOLS:
        case SHOW_FINISHED:
        case SHUTDOWN:
        case UNKNOWN:
          break;
        }
      }
    }

    if (fds[1].revents & POLLIN) {
      if (signals_read(signal_fd, &signal) >= 0) {
        switch (signal.cause) {
        case STOPPED:
          jobs_stopped(signal.pid);
          break;
        case CONTINUED:
          jobs_continued(signal.pid);
          break;
        case EXITED:
          jobs_exited(signal.pid);
          break;
        }
      }
    }
  }

  close(signal_fd);
  free(jobs);
  free(cmd_buffer);
  close_pipes();

  exit(0);
}

int setup_signals() {
  // TODO: also handle SIGKILL
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

  return signal_fd;
}

int init_pipes(int id) {
  char str[32];
  sprintf(str, "pool_%d_in", id);

  PIPEIN = open(str, O_RDWR | O_CLOEXEC | O_NONBLOCK);
  if (PIPEIN < 0) {
    return -1;
  }

  sprintf(str, "pool_%d_out", id);

  PIPEOUT = fopen(str, "r+e");
  if (PIPEOUT == NULL) {
    close(PIPEIN);
    return -1;
  }

  return 0;
}
void close_pipes() {
  close(PIPEIN);
  fclose(PIPEOUT);
}