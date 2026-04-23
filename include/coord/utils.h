#ifndef UTILS_H
#define UTILS_H

#include <sys/types.h>

typedef enum { STOPPED, CONTINUED, EXITED } Cause;

typedef struct {
  Cause cause;
  pid_t pid;
} SignalInfo;

ssize_t read_blocking(int _fd, void *_buf, size_t _nbytes);
int count_words(const char *args);
int decode_args(char *raw, char **argv);
int decode_signal(int fd, SignalInfo *info);

#endif