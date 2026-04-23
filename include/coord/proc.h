#ifndef PROC_H
#define PROC_H

#include <stdio.h>
#include <string.h>

#include "pool.h"

extern FILE *pipeout;

#define send_job(job)                                                          \
  {                                                                            \
    PoolMessage msg = {0, sizeof(Job)};                                        \
    fwrite(&msg, sizeof(PoolMessage), 1, pipeout);                             \
    fflush(pipeout);                                                           \
    fwrite(job, sizeof(Job), 1, pipeout);                                      \
    fflush(pipeout);                                                           \
  }

#define sendf(fmt, ...)                                                        \
  {                                                                            \
    /* NO NEED TO ACCOUNT FOR NULL BYTE */                                     \
    int length = snprintf(NULL, 0, fmt __VA_OPT__(, ) __VA_ARGS__);            \
    PoolMessage msg = {1, length};                                             \
    fwrite(&msg, sizeof(PoolMessage), 1, pipeout);                             \
    fflush(pipeout);                                                           \
    fprintf(pipeout, fmt __VA_OPT__(, ) __VA_ARGS__);                          \
    fflush(pipeout);                                                           \
  }

/**
 * @brief Main entry point of the pool process
 * @param id id of the process
 * @return pool process exit status
 */
int proc_main(int id);

#endif