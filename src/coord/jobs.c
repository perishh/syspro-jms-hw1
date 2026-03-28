#include "jobs.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "globals.h"
#include "list.h"
#include "outputs.h"
#include "utils.h"

int key = 1; // starts from 1 to handle invalid atoi

typedef struct {
  int id;
  pid_t pid; // 0 indicates empty slot
  int running;
  time_t timestamp;
} Job;

#define IS_JOB_EMPTY(job) ((job).pid == 0)
#define CLEAR_JOB(job) ((job).pid = 0)

typedef struct {
  int active;
  Job jobs[]; // FAM
} Pool;

LinkedList pools;
LinkedList finished;

void jobs_init() {
  ll_init(&pools);
  ll_init(&finished);
}

int jobs_add(const Job *job) {
  Pool *pool = NULL;

  FOR_EACH(pools, n) {
    Pool *p = (Pool *)n->data;
    if (p->active < jobs_pool) {
      pool = p;
      break;
    }
  }

  if (pool == NULL) {
    // Create new pool
    pool = calloc(1, sizeof(Pool) + (sizeof(Job) * jobs_pool));
    if (pool == NULL) {
      return -1;
    }
    if (ll_push(&pools, pool) < 0) {
      free(pool);
      return -1;
    }
  }

  // Search for available job slot
  for (int i = 0; i < jobs_pool; i++) {
    if (IS_JOB_EMPTY(pool->jobs[i])) {
      pool->jobs[i] = *job;
      break;
    }
  }
  pool->active++;

  // TODO: Check with gdb

  return 0;
}

// Arguments given by coord are null terminated
int jobs_submit(char *cmd_args) {
  // Delimiter is Space or \n (or NULL)
  int argc = count_words(cmd_args);

  // TODO: Consider malloc
  char *argv[argc + 1]; // Account for terminating NULL

  // strtok(3)
  char *program = strtok(cmd_args, " \n");
  if (program == NULL) {
    return -1;
  }

  int i = 0;
  argv[i++] = program;
  while ((argv[i++] = strtok(NULL, " \n")) != NULL) {
  }

  Job job;
  job.id = key++;
  job.running = 1;
  // time(2)
  job.timestamp = time(NULL);

  // fork(2)
  job.pid = fork();
  if (job.pid == 0) {
    // Child process

    // Change working directory
    if (outputs_cd(job.id, job.timestamp) < 0) {
      perror("cd to outputs");
      return 1;
    }

    // Redirect stdout & stderr to files
    if (outputs_redirect(job.id) < 0) {
      perror("redirect_outputs");
      return 1;
    }

    // exec(3)
    int ret = execvp(program, argv);
    if (ret < 0) {
      // Failed to exec
      perror("execv");
      return ret;
    }
  }

  if (job.pid < 0) {
    // Failed to fork
    return -1;
  }

  if (jobs_add(&job) < 0) {
    return -1;
  }

  printf("JobID: %d, PID: %d\n", job.id, job.pid);

  return 0;
}

void jobs_show_finished() {
  printf("Finished jobs:\n");

  Job *j;
  FOR_EACH(finished, n) {
    j = (Job*) n->data;
    printf("JobID %d\n", j->id);
  }
}

void jobs_show_pools() {
  // TODO: Does the number include suspended jobs?
}

void jobs_show_active() {
  printf("Active jobs:\n");

  Pool* p;
  FOR_EACH(pools, n) {
    p = (Pool*) n->data;
    for(int i = 0;i<jobs_pool;i++) {
      Job *j = &p->jobs[i];
      if(!IS_JOB_EMPTY(*j) && j->running) {
        printf("JobID %d\n", j->id);
      }
    }
  }
}

int jobs_status(int id) {
  Job *job = NULL;
  int has_finished = 0;

  Pool *p;
  FOR_EACH(pools, n) {
    p = (Pool *)n->data;
    for (int i = 0; i < jobs_pool; i++) {
      if (p->jobs[i].id == id) {
        job = &p->jobs[i];
        break;
      }
    }
  }

  has_finished = 1;
  FOR_EACH(finished, n) {
    Job *j = (Job *)n->data;
    if (j->id == id) {
      job = j;
      break;
    }
  }

  if (job == NULL) {
    printf("JobID %d not found\n", id);
    return -1;
  }

  if (has_finished) {
    printf("JobID %d Status:\tFinished\n", id);
  } else if (job->running) {
    time_t now = time(NULL);
    int elapsed = now - job->timestamp;
    printf("JobID %d Status:\tActive (running for %d sec)\n", id, elapsed);
  } else {
    printf("JobID %d Status:\tSuspended\n", id);
  }

  return 0;
}

void jobs_status_all(int n) {
  time_t now = time(NULL);

  Job *j;
  FOR_EACH(finished, node) {
    j = (Job *)node->data;
    int elapsed = now - j->timestamp;
    if (elapsed <= n) {
      // TODO: Maybe optimize
      printf("JobID %d Status:\tFinished\n", j->id);
    }
  }

  Pool *p;
  FOR_EACH(pools, node) {
    p = (Pool *)node->data;
    for (int i = 0; i < jobs_pool; i++) {
      j = &p->jobs[i];
      int elapsed = now - j->timestamp;
      if (elapsed <= n) {
        if (j->running) {
          printf("JobID %d Status:\tActive (running for %d sec)\n", j->id,
                 elapsed);
        } else {
          printf("JobID %d Status:\tSuspended\n", j->id);
        }
      }
    }
  }
}

int jobs_suspend(int id) {
  Pool *p;
  FOR_EACH(pools, n) {
    p = (Pool *)n->data;
    for (int i = 0; i < jobs_pool; i++) {
      if (p->jobs[i].id == id) {
        if (!p->jobs[i].running) {
          printf("Job already suspended\n");
          return -1;
        }

        // kill(2)
        if (kill(p->jobs[i].pid, SIGSTOP) < 0) {
          printf("Couldn't send signal to job\n");
          return -1;
        }

        printf("Sent suspend signal to JobID %d\n", id);
        return 0;
      }
    }
  }

  printf("JobID %d not found\n", id);
  return -1;
}

int jobs_resume(int id) {
  Pool *p;
  FOR_EACH(pools, n) {
    p = (Pool *)n->data;
    for (int i = 0; i < jobs_pool; i++) {
      if (p->jobs[i].id == id) {
        if (p->jobs[i].running) {
          printf("Job already running\n");
          return -1;
        }

        // kill(2)
        if (kill(p->jobs[i].pid, SIGCONT) < 0) {
          printf("Couldn't send signal to job\n");
          return -1;
        }

        printf("Sent resume signal to JobID %d\n", id);
        return 0;
      }
    }
  }

  printf("JobID %d not found\n", id);
  return -1;
}

int jobs_stopped(pid_t pid) {
  // TODO: Should suspended jobs remain on the pool?
  Pool *p;
  FOR_EACH(pools, n) {
    p = (Pool *)n->data;
    for (int i = 0; i < jobs_pool; i++) {
      if (p->jobs[i].pid == pid) {
        p->jobs[i].running = 0;
        return 0;
      }
    }
  }

  return -1;
}

int jobs_continued(pid_t pid) {
  Pool *p;
  FOR_EACH(pools, n) {
    p = (Pool *)n->data;
    for (int i = 0; i < jobs_pool; i++) {
      if (p->jobs[i].pid == pid) {
        p->jobs[i].running = 1;
        return 0;
      }
    }
  }

  return -1;
}

int jobs_exited(pid_t pid) {
  Job *job = NULL;

  Pool *p;
  FOR_EACH(pools, n) {
    p = (Pool *)n->data;
    for (int i = 0; i < jobs_pool; i++) {
      if (p->jobs[i].pid == pid) {
        job = &p->jobs[i];
        break;
      }
    }
  }

  if (job == NULL) {
    return -1;
  }

  Job *finished_job = malloc(sizeof(Job));
  if (finished_job == NULL) {
    return -1;
  }

  *finished_job = *job;

  finished_job->running = 0;
  if (ll_push(&finished, finished_job) < 0) {
    return -1;
  }

  CLEAR_JOB(*job);
  p->active--;

  // TODO: Should delete pool if jobs are empty?

  return 0;
}

void jobs_shutdown() {

}

void jobs_free() {
  // TODO: Free pool & job struct memory
  ll_free(&pools);
  ll_free(&finished);
}