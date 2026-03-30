#ifndef UTILS_H
#define UTILS_H

#include <sys/types.h>
#include <stdio.h>

typedef enum { STOPPED, CONTINUED, EXITED } Cause;

typedef struct {
  Cause cause;
  pid_t pid;
} SignalInfo;

ssize_t read_blocking(int __fd, void *__buf, size_t __nbytes);
int count_words(const char *args);
int decode_args(char *raw, char **argv);
int decode_signal(int fd, SignalInfo *info);

#endif