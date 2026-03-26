#include "jobs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "globals.h"
#include "list.h"

#define DATETIME_SIZE 16 // Including \0

int count_words(const char *args);

int key = 0;

typedef struct {
  int id;
  pid_t pid;
  time_t timestamp;
} Job;

typedef struct {
  int active;
  Job jobs[]; // FAM
} Pool;

LinkedList pools;

int cd_to_outputs(int id, time_t timestamp) {
  char *buffer = malloc(64);
  if (buffer == NULL) {
    return -1;
  }

  // getpid(2)
  pid_t pid = getpid();

  // printf(3)
  int base;
  if ((base = snprintf(buffer, 64 - DATETIME_SIZE, "outputs_%d_%d_", id,
                       pid)) >= 64 - DATETIME_SIZE) {
    free(buffer);
    return -1;
  }

  // strftime(3)
  // Begin writing on null byte of previous snprintf
  if (strftime(buffer + base, DATETIME_SIZE, "%Y%m%d_%H%M%S",
               localtime(&timestamp)) == 0) {
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

void jobs_init() { ll_init(&pools); }

int jobs_add(const Job *job) {
  // TODO: Check for available pool

  // Create new pool
  Pool *pool = calloc(1, sizeof(Pool) + (sizeof(Job) * jobs_pool));
  if (pool == NULL) {
    return -1;
  }
  pool->jobs[pool->active++] = *job;

  if (ll_push(&pools, pool) < 0) {
    free(pool);
    return -1;
  }

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
    if (cd_to_outputs(job.id, job.timestamp) < 0) {
      perror("cd to outputs");
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
    perror("fork");
    return -1;
  }

  return jobs_add(&job);
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