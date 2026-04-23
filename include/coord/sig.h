#ifndef SIG_H
#define SIG_H

#include <sys/types.h>

extern int SIG_FILENO;

/**
 * @brief Initializes signal handling
 *
 * Blocks default handling of SICHLD, SIGINT, SIGTERM,
 * creates signalfd to handle them, adds it to epoll.
 *
 * @return 0 on success, -1 otherwise
 */
int sig_init();

/**
 * @brief Closes signalfd
 */
void sig_free();

#endif