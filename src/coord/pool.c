#include "pool.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include "args.h"
#include "cmd.h"
#include "command.h"
#include "globals.h"
#include "io.h"
#include "list.h"
#include "polling.h"
#include "proc.h"
#include "sig.h"

#define BUFFER_SIZE 4096

typedef struct {
  int id;
  pid_t pid;
  int jobs;
} Pool;

static char *buffer = NULL;

int pool_redirect(int fd) {
  ssize_t nread = read(fd, buffer, BUFFER_SIZE);
  if (nread <= 0) {
    return nread;
  }

  return write(JMSOUT_FILENO, buffer, nread);
}

int pool_io_init(int id) {
  char str[32];
  sprintf(str, "pool_%d_in", id);

  unlink(str);
  if (mkfifo(str, MODE_RW) < 0) {
    return -1;
  }

  sprintf(str, "pool_%d_out", id);

  unlink(str);
  if (mkfifo(str, MODE_RW) < 0) {
    sprintf(str, "pool_%d_in", id);
    unlink(str);
    return -1;
  }

  // Input to coord output to pool
  int in = open(str, O_RDWR | O_NONBLOCK | O_CLOEXEC);
  if (in < 0) {
    unlink(str);
    sprintf(str, "pool_%d_in", id);
    unlink(str);
    return -1;
  }

  if (polling_add(in) < 0) {
    close(in);
    unlink(str);
    sprintf(str, "pool_%d_in", id);
    unlink(str);
    return -1;
  }

  return 0;
}

int pool_send(const Command *cmd, int id) {
  char str[32];
  sprintf(str, "pool_%d_in", id);

  int out = open(str, O_RDWR | O_NONBLOCK | O_CLOEXEC);
  if (out < 0) {
    return -1;
  }

  if (write(out, cmd, sizeof(Command)) <= 0) {
    close(out);
    return -1;
  }

  if (write(out, cmd->args, cmd->len + 1) < 0) {
    close(out);
    return -1;
  }

  return 0;
}

LinkedList pools;

int pool_key = 1;

int pool_start() {
  Pool *pool = malloc(sizeof(Pool));
  if (pool == NULL) {
    return -1;
  }

  pool->id = pool_key++;
  pool->jobs = 0;

  pool_io_init(pool->id);

  pool->pid = fork();
  if (pool->pid == 0) {
    int id = pool->id;

    free(pool);
    cmd_free();
    polling_free();
    io_close();
    sig_free();
    pool_free();

    _exit(proc_main(id));
  }

  if (ll_push(&pools, pool) < 0) {
    free(pool);
    return -1;
  }

  return 0;
}

int pool_init() {
  ll_init(&pools);
  buffer = malloc(BUFFER_SIZE);
  if (buffer == NULL) {
    return -1;
  }

  return 0;
}

void pool_free() {
  ll_free(&pools);
  free(buffer);
}

int job_key = 1;
int pool_submit(Command *cmd) {
  if (pools.size == 0) {
    pool_start();
  }

  Pool *pool = (Pool *)pools.front->data;
  if (pool->jobs >= get_jobs_pool()) {
    // Pool full, create new
    pool_start();
    pool = (Pool *)pools.front->data;
  }

  // Set job id
  cmd->data = job_key++;

  if (pool_send(cmd, pool->id) < 0) {
    free(pool);
    return -1;
  }

  pool->jobs++;
  return 0;
}

void pool_broadcast(Command *cmd) {
  Pool *p;
  FOR_EACH(pools, node) {
    p = (Pool *)node->data;
    pool_send(cmd, p->id);
  }
}

void pool_show() {
  write(JMSOUT_FILENO, "Pool & NumOfJobs:\n", 19);
  Pool *p;
  FOR_EACH(pools, node) {
    p = (Pool *)node->data;
    int n = snprintf(buffer, BUFFER_SIZE, "%d %d\n", p->pid, p->jobs);
    write(JMSOUT_FILENO, buffer, n + 1); // For \0
  }
}