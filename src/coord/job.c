#include "job.h"

#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "args.h"
#include "globals.h"
#include "proc.h"
#include "utils.h"

#define DATETIME_SIZE 16 // Including \0

int job_proc_cd(int id, time_t timestamp) {
  char *buffer = malloc(64);
  if (buffer == NULL) {
    return -1;
  }

  // getpid(2)
  pid_t pid = getpid();

  // printf(3)
  int base = snprintf(buffer, 64 - DATETIME_SIZE, "outputs_%d_%d_", id, pid);
  if (base >= 64 - DATETIME_SIZE || base < 0) {
    free(buffer);
    return -1;
  }

  // strftime(3)
  // Begin writing on null byte of previous snprintf
  if (strftime(buffer + base, DATETIME_SIZE, "%Y%m%d_%H%M%S",
               localtime(&timestamp)) <= 0) {
    free(buffer);
    return -1;
  }

  // mkdir(2), inode(7)
  // Grant all permissions
  if (mkdir(buffer, S_IRWXU | S_IRWXG | S_IRWXO) < 0) {
    free(buffer);
    return -1;
  }

  // chdir(2)
  if (chdir(buffer) < 0) {
    // rmdir(2)
    rmdir(buffer);
    free(buffer);
    return -1;
  }

  free(buffer);
  return 0;
}

int job_proc_redirect(int id) {
  char path[32];

  int ret = snprintf(path, 32, "stdout_%d", id);
  if (ret >= 32 || ret < 0) {
    perror("snprintf");
    return -1;
  }

  int out_file = open(path, O_WRONLY | O_CREAT | O_TRUNC, MODE_RW);
  if (out_file < 0) {
    return -1;
  }

  if (dup2(out_file, STDOUT_FILENO) < 0) {
    close(out_file);
    unlink(path);
    return -1;
  }

  ret = snprintf(path, 32, "stderr_%d", id);
  if (ret >= 32 || ret < 0) {
    close(out_file);
    return -1;
  }

  int err_file = open(path, O_WRONLY | O_CREAT | O_TRUNC, MODE_RW);
  if (err_file < 0) {
    close(out_file);
    return -1;
  }

  if (dup2(err_file, STDERR_FILENO) < 0) {
    close(out_file);
    close(err_file);
    unlink(path);
    return -1;
  }

  return 0;
}

static int size;
static Job *jobs;

int get_job_count() { return size; }

int job_add(int id, char *raw) {
  if (size >= get_jobs_pool()) {
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
    if (job_proc_cd(job->id, job->timestamp) < 0) {
      _exit(1);
    }

    // Redirect stdout & stderr to files
    if (job_proc_redirect(job->id) < 0) {
      _exit(1);
    }

    // exec(3)
    int ret = execvp(argv[0], argv);
    if (ret < 0) {
      _exit(1);
    }
    _exit(0);
  }

  if (job->pid < 0) {
    size--;
    return -1;
  }

  sendf("JobID: %d, PID: %d\n", job->id, job->pid);
  return 0;
}

void job_show_active() {
  for (int i = 0; i < size; i++) {
    Job *j = &jobs[i];
    if (!j->finished && !j->suspended) {
      sendf("JobID %d\n", j->id);
    }
  }
}

void job_show_finished() {
  for (int i = 0; i < size; i++) {
    Job *j = &jobs[i];
    if (j->finished) {
      sendf("JobID %d\n", j->id);
    }
  }
}

int job_status(int id) {
  Job *j = NULL;
  for (int i = 0; i < size; i++) {
    if (jobs[i].id == id) {
      j = &jobs[i];
    }
  }

  if (j == NULL) {
    return -1;
  }

  if (j->suspended) {
    sendf("JobID %d Status:\tSuspended\n", id);
  } else if (!j->finished) {
    time_t now = time(NULL);
    long elapsed = now - (j->timestamp);
    sendf("JobID %d Status:\tActive (running for %ld sec)\n", id, elapsed);
  }

  return 0;
}

void job_status_all(int n) {
  time_t now = time(NULL);

  for (int i = 0; i < size; i++) {
    Job *j = &jobs[i];
    long elapsed = now - j->timestamp;
    if (n <= 0 || elapsed <= n) {
      if (j->suspended) {
        sendf("JobID %d Status:\tSuspended\n", j->id);
      } else if (!j->finished) {
        sendf("JobID %d Status:\tActive (running for %ld sec)\n", j->id,
              elapsed);
      }
    }
  }
}

int job_continued(pid_t pid) {
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

int job_stopped(pid_t pid) {
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

int job_exited(pid_t pid) {
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
  send_job(job);
  return 0;
}

int job_resume(int id) {
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

  sendf("Sent resume signal to JobID %d\n", id);
  return 0;
}

int job_suspend(int id) {
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

  sendf("Sent suspend signal to JobID %d\n", id);
  return 0;
}

int job_init() {
  jobs = malloc(sizeof(Job) * get_jobs_pool());
  if (jobs == NULL) {
    return -1;
  }

  return 0;
}

int job_shutdown() {
  int in_progress = 0;
  for (int i = 0; i < size; i++) {
    Job *job = &jobs[i];
    if (!job->finished) {
      in_progress++;
      kill(job->pid, SIGTERM);
    }
  }
  return in_progress;
}

void job_free() { free(jobs); }