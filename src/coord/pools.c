#include "pools.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "globals.h"
#include "list.h"
#include "utils.h"

// TODO: REDESIGN
// EACH POOL IS A SEPARATE PROCESS
// ID DELEGATION

typedef struct {
  int id;
  pid_t pid;
  int active;
} Pool;

LinkedList pools;

int key = 1;

void pools_init() {
  ll_init(&pools);
}

int pools_start(int argc, char** argv) {
  Pool* pool = malloc(sizeof(Pool));
  if(pool == NULL) {
    return -1;
  }

  pool->id = key++;
  pool->active = 0;

  char str[32];
  sprintf(str, "pool_%d_in", pool->id);

  if(mkfifo(str, MODE_RW) < 0) {
    free(pool);
    return -1;
  }

  sprintf(str, "pool_%d_out", pool->id);
  
  if(mkfifo(str, MODE_RW) < 0) {
    sprintf(str, "pool_%d_in", pool->id);
    unlink(str);
    free(pool);
    return -1;
  }

  pool->pid = fork();
  if(pool->pid == 0) {
    // Pool process
    // TODO: Close opened resources
  }

  if(pool->pid < 0) {
    sprintf(str, "pool_%d_in", pool->id);
    unlink(str);
    free(pool);
    return -1;
  }





}


int pools_enqueue(char *raw) {
  int argc = count_words(raw);
  char *argv[argc + 1]; // Account for terminating NULL; TODO: Consider malloc
  if(decode_args(raw, argv) < 0) {
    return -1;
  }

  // TODO: Check for available pools

}

void pools_free() {
  // TODO: Free pool data
  ll_free(&pools);
}