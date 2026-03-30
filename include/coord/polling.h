#ifndef POLLING_H
#define POLLING_H

#include <sys/epoll.h>

int polling_init();
int polling_add(int fd);
int polling_remove(int fd);
int polling_wait(struct epoll_event **events);
void polling_free();

#endif