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
#include "pipes.h"
#include "utils.h"

typedef struct {
  int id;
  pid_t pid;
  int active;
} Pool;

LinkedList pools;

int pool_key = 1;
int job_key = 1;

Command *cmd_buffer;

int pools_init() {
  if ((cmd_buffer = malloc(sizeof(Command))) == NULL) {
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

Pool *find_or_start() {
  Pool *pool = NULL;
  FOR_EACH(pools, n) {
    Pool *p = (Pool *)n->data;
    if (p->active < jobs_pool) {
      pool = p;
      break;
    }
  }

  if (pool == NULL) {
    if (pools_start() < 0) {
      return NULL;
    }
    pool = (Pool *)pools.front->data;
  }

  return pool;
}

int send_and_receive(const Pool *pool, const char *args) {
  char str[32];

  // TODO: Race condition a pool might have exited by
  // the time the request to submit a new job is sent
  sprintf(str, "pool_%d_out", pool->id);
  int in = open(str, O_RDONLY | O_NONBLOCK);
  if (in < 0) {
    return -1;
  }

  sprintf(str, "pool_%d_in", pool->id);
  int out = open(str, O_WRONLY);
  if (out < 0) {
    close(in);
    return -1;
  }

  if (write(out, cmd_buffer, sizeof(Command)) < 0) {
    close(in);
    close(out);
    return -1;
  }

  if (write(out, args, cmd_buffer->len + 1) < 0) {
    close(in);
    close(out);
    return -1;
  }

  // Read first blocking
  ssize_t nread = read_blocking(in, str, 32);
  if (nread > 0) {
    do {
      write(JMSOUT_FILENO, str, nread);
    } while ((nread = read(in, str, 32)) > 0);
  }

  // No need to check for redirection errors

  close(in);
  close(out);

  return 0;
}

int pools_enqueue(int len, char *args) {
  Pool *pool = find_or_start();
  if (pool == NULL) {
    // TODO: FIX Reached here
    return -1;
  }

  pool->active++;

  cmd_buffer->action = SUBMIT;
  cmd_buffer->data = job_key++;
  cmd_buffer->len = len;

  return send_and_receive(pool, args);
}

void pools_free() {
  // TODO: Free pool data
  free(cmd_buffer);
  ll_free(&pools);
}