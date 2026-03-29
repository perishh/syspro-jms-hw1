#include "pools.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "command.h"
#include "globals.h"
#include "jobs.h"
#include "list.h"
#include "utils.h"

typedef struct {
  int id;
  pid_t pid;
  int active;
} Pool;

LinkedList pools;

int pool_key = 1;
int job_key = 1;

Command* cmd_buffer;

int pools_init() {
  if((cmd_buffer = malloc(sizeof(Command))) == NULL) {
    return -1;
  }
  ll_init(&pools);
  return 0;
}

int pools_start() {
  Pool *pool = malloc(sizeof(Pool));
  if (pool == NULL) {
    return -1;
  }

  pool->id = pool_key++;
  pool->active = 0;

  char str[32];
  sprintf(str, "pool_%d_in", pool->id);

  unlink(str);
  if (mkfifo(str, MODE_RW) < 0) {
    free(pool);
    return -1;
  }

  sprintf(str, "pool_%d_out", pool->id);

  unlink(str);
  if (mkfifo(str, MODE_RW) < 0) {
    sprintf(str, "pool_%d_in", pool->id);
    unlink(str);
    free(pool);
    return -1;
  }

  pool->pid = fork();
  if (pool->pid == 0) {
    // Pool process
    // TODO: Close opened resources
    jobs_init(pool->id);
  }

  if (pool->pid < 0) {
    unlink(str);
    sprintf(str, "pool_%d_in", pool->id);
    unlink(str);
    free(pool);
    return -1;
  }

  if (ll_push(&pools, pool) < 0) {
    unlink(str);
    sprintf(str, "pool_%d_in", pool->id);
    unlink(str);
    free(pool);
    return -1;
  }

  return 0;
}

Pool* find_or_start() {
  Pool* pool = NULL;
  FOR_EACH(pools, n) {
    Pool* p = (Pool*) n->data;
    if(p->active < jobs_pool) {
      pool = p;
      break;
    }
  }

  if(pool == NULL) {
    if(pools_start() < 0) {
      return NULL;
    }
    pool = (Pool*) pools.front->data;
  }

  return pool;
}

int pools_enqueue(int len, char *args) {
  Pool* pool = find_or_start();
  if(pool == NULL) {
    // TODO: FIX Reached here
    return -1;
  }

  char str[32];

  
  sprintf(str, "pool_%d_out", pool->id);
  int in = open(str, O_RDONLY | O_NONBLOCK);
  if(in < 0) {
    return -1;
  }

  printf("Opened pipe to pool\n");
  
  sprintf(str, "pool_%d_in", pool->id);
  int out = open(str, O_WRONLY);
  if(out < 0) {
    perror("open pool in");
    printf("Failed to open pipe from pool %s\n", str);
    close(in);
    return -1;
  }

  printf("Opened pipe from pool\n");
  
  cmd_buffer->action = SUBMIT;
  cmd_buffer->data = job_key++;
  cmd_buffer->len = len;

  pool->active++;

  if(write(out, cmd_buffer, sizeof(Command)) < 0) {
    close(in);
    close(out);
    return -1;
  }

  printf("Written command\n");

  if(write(out, args, cmd_buffer->len + 1) < 0) {
    close(in);
    close(out);
    return -1;
  }

  printf("Written args\n");

  // Redirect to console (duped stdout)
  ssize_t nread;
  while((nread = read_blocking(in, str, 32)) > 0) {
    printf("%.*s\n", (int) nread, str);
    write(STDOUT_FILENO, str, nread);
  }
  // No need to check for redirection errors

  printf("Read output\n");

  close(in);
  close(out);

  return 0;
}

void pools_free() {
  // TODO: Free pool data
  free(cmd_buffer);
  ll_free(&pools);
}