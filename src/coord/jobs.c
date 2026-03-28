#include "jobs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "globals.h"
#include "outputs.h"
#include "list.h"

int count_words(const char *args);

int key = 0;

typedef struct {
  int id;
  pid_t pid;            // 0 indicates empty slot
  time_t timestamp;
} Job;

#define IS_JOB_EMPTY(job) (job.pid == 0)

typedef struct {
  int active;
  Job jobs[];           // FAM
} Pool;

LinkedList pools;

void jobs_init() { ll_init(&pools); }

int jobs_add(const Job *job) {
  Pool* pool = NULL;
  for(Node *n = pools.front; n != NULL; n = n->next) {
    Pool* p = (Pool*) n->data;
    if(p->active < jobs_pool) {
      pool = p;
      break;
    }
  }

  if(pool == NULL) {
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
  for(int i = 0;i<jobs_pool;i++) {
    if(IS_JOB_EMPTY(pool->jobs[i])) {
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

  if(jobs_add(&job) < 0){
    return -1;
  }

  // TODO: Print output

  return 0;
}

void jobs_show_finished() {
  // TODO
}

void jobs_show_pools() {
  // TODO
}

void jobs_show_active() {
  // TODO
}

void jobs_status(int id){
  // TODO
}

void jobs_status_all(int n) {
  // TODO
}

void jobs_suspend(int id) {
  // TODO
}

void jobs_resume(int id) {
  // TODO
}

void jobs_stopped(pid_t pid) {
  // TODO
}

void jobs_exited(pid_t pid) {
  // TODO
}

void jobs_continued(pid_t pid) {
  // TODO
}

void jobs_free() {
  // TODO: Free pool struct memory
  ll_free(&pools);
}

int count_words(const char *args) {
  int in_delim = 1;
  int count = 0;
  int i = 0;
  char current;
  do {
    current = args[i++];
    if (current == ' ' || current == '\n' || current == '\0') {
      if (!in_delim) {
        in_delim = 1;
        count++;
      }
    } else {
      in_delim = 0;
    }
  } while (current != '\0');
  return count;
}