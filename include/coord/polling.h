#ifndef POLLING_H
#define POLLING_H

#include <sys/epoll.h>

/**
 * @return 0 on success, -1 otherwise
 */
int polling_init();

/**
 * @return 0 on success, -1 otherwise
 */
int polling_add(int fd);

/**
 * @return 0 on success, -1 otherwise
 */
int polling_remove(int fd);

/**
 * @return 0 on success, -1 otherwise
 */
int polling_wait(struct epoll_event **events);

/**
 * @brief Closes and frees epoll resources
 */
void polling_free();

#endif