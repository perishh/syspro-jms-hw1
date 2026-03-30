#ifndef PROC_H
#define PROC_H

#include <stdio.h>

extern FILE *pipeout;

#define sendf(fmt, ...)                                                        \
  {                                                                            \
    fprintf(pipeout, fmt __VA_OPT__(, ) __VA_ARGS__);                   \
    fflush(pipeout);                                                    \
  }

int proc_main(int id);

#endif