#ifndef SIGNALS_H
#define SIGNALS_H

#include <sys/types.h>

typedef enum { STOPPED, CONTINUED, EXITED } Cause;

typedef struct {
  Cause cause;
  pid_t pid;
} SignalInfo;

int signals_setup(int epoll_fd);
int signals_read(int signal_fd, SignalInfo *info);

#endif