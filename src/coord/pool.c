#include "pool.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "args.h"
#include "cmd.h"
#include "command.h"
#include "globals.h"
#include "io.h"
#include "job.h"
#include "list.h"
#include "polling.h"
#include "proc.h"
#include "sig.h"
#include "utils.h"

#define INITIAL_BUFFER_SIZE 4096

typedef struct {
  int id;
  pid_t pid;
  int fd;
  int jobs;
} Pool;

static char *buffer = NULL;
static ssize_t buffer_size;

LinkedList finished_jobs;

long pool_redirect(int fd) {
  static PoolMessage msg;
  ssize_t nread = read(fd, &msg, sizeof(PoolMessage));
  if (nread != sizeof(PoolMessage)) {
    return nread;
  }

  if (buffer_size < msg.length) {
    char *temp = realloc(buffer, msg.length);
    if (temp == NULL) {
      return -1;
    }
    buffer_size = msg.length;
    buffer = temp;
  }
  nread = read_blocking(fd, buffer, msg.length);

  if (msg.isText) {
    return write(JMSOUT_FILENO, buffer, nread);
  }

  for (char *i = buffer; i < buffer + msg.length; i += sizeof(Job)) {
    Job *j = malloc(sizeof(Job));
    if (j != NULL) {
      *j = *((Job *)i);
      ll_push(&finished_jobs, j);
    }
  }

  return 0;
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

  return in;
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

  if (cmd->len > 0) {
    if (write(out, cmd->args, cmd->len + 1) < 0) {
      close(out);
      return -1;
    }
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
  pool->fd = pool_io_init(pool->id);

  if (pool->fd < 0) {
    free(pool);
    return -1;
  }

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
  ll_init(&finished_jobs);
  buffer_size = INITIAL_BUFFER_SIZE;
  buffer = malloc(INITIAL_BUFFER_SIZE);
  if (buffer == NULL) {
    return -1;
  }

  return 0;
}

void pool_free() {
  FOR_EACH(pools, node) { free(node->data); }
  FOR_EACH(finished_jobs, node) { free(node->data); }
  ll_free(&pools);
  ll_free(&finished_jobs);
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

void pool_status(Command *cmd) {
  int id = atoi(cmd->args);
  int found = 0;
  FOR_EACH(finished_jobs, node) {
    Job *j = (Job *)node->data;
    if (j->id == id) {
      int size =
          snprintf(buffer, buffer_size, "JobID %d Status:\tFinished\n", id);
      if (size < buffer_size) {
        write(JMSOUT_FILENO, buffer, size + 1);
      }
      found = 1;
      break;
    }
  }
  if (!found) {
    pool_broadcast(cmd);
  }
}

void pool_status_all(Command *cmd) {
  int n = 0;
  if (cmd->len > 0) {
    n = atoi(cmd->args);
  }
  time_t now = time(NULL);

  FOR_EACH(finished_jobs, node) {
    Job *j = (Job *)node->data;
    long elapsed = now - j->timestamp;
    if (n <= 0 || elapsed <= n) {
      int size =
          snprintf(buffer, buffer_size, "JobID %d Status:\tFinished\n", j->id);
      if (size < buffer_size) {
        write(JMSOUT_FILENO, buffer, size + 1);
      }
    }
  }
  pool_broadcast(cmd);
}

void pool_finished() {
  write(JMSOUT_FILENO, "Finished jobs:\n", 16);

  Job *j;
  FOR_EACH(finished_jobs, node) {
    j = (Job *)node->data;
    int n = snprintf(buffer, buffer_size, "JobID %d\n", j->id);
    if (n < buffer_size) {
      write(JMSOUT_FILENO, buffer, n + 1); // For \0
    }
  }
}

void pool_show() {
  write(JMSOUT_FILENO, "Pool & NumOfJobs:\n", 19);
  Pool *p;
  FOR_EACH(pools, node) {
    p = (Pool *)node->data;
    int n = snprintf(buffer, buffer_size, "%d %d\n", p->pid, p->jobs);
    if (n < buffer_size) {
      write(JMSOUT_FILENO, buffer, n + 1); // For \0
    }
  }
}

// int total_exited = 0;
int in_progress_at_shutdown = 0;

int pool_exited(pid_t pid) {
  Node *n = NULL;
  FOR_EACH(pools, node) {
    Pool *p = (Pool *)node->data;
    if (p->pid == pid) {
      n = node;
      break;
    }
  }

  if (n != NULL) {
    Pool *p = (Pool *)n->data;
    polling_remove(p->fd);
    close(p->fd);

    char str[32];
    sprintf(str, "pool_%d_in", p->id);
    unlink(str);
    sprintf(str, "pool_%d_out", p->id);
    unlink(str);

    free(p);
    ll_remove(&pools, n);
  }
  return pools.size == 0;
}

int pool_shutdown() {
  if (pools.size == 0) {
    // NO POOLS ACTIVE
    return 1;
  }
  in_progress_at_shutdown = job_key - finished_jobs.size - 1;
  FOR_EACH(pools, node) {
    Pool *p = (Pool *)node->data;
    kill(p->pid, SIGTERM);
  }
  return 0;
}

void pool_print_info() {
  int n = snprintf(buffer, buffer_size,
                   "Served %d jobs, %d were still in progress\n",
                   finished_jobs.size, in_progress_at_shutdown);
  if (n < buffer_size) {
    write(JMSOUT_FILENO, buffer, n + 1); // For \0
  }
}