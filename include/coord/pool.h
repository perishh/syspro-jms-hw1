#ifndef POOL_H
#define POOL_H

#include "command.h"
#include <fcntl.h>

// Struct used for pools to communicate with coord
// to distinguish between string and (finished) job
// messages
typedef struct {
  int isText;
  int length;
} PoolMessage;

/**
 * @brief Initializes lists & buffer needed for
 * pool management
 *
 * @return 0 on success, -1 otherwise
 */
int pool_init();

/**
 * @brief Deallocates pool & finished job lists & buffer
 */
void pool_free();

/**
 * @brief Handles message from pool
 *
 * Reads PoolMessage from file descriptor, if isText
 * redirects string to jms_out, otherwise saves finished
 * job to linked list.
 *
 * @param fd pool file descriptor
 * @return < 0 on error, >= 0 otherwise
 */
long pool_redirect(int fd);

/**
 * @brief Delegates job to pool
 *
 * Finds first available pool or starts new one
 * and forwards the submit command to it.
 *
 * @param cmd pointer to the submit command
 * @return 0 on success, -1 otherwise
 */
int pool_submit(Command *cmd);

/**
 * @brief Broadcasts command to every pool
 * @param cmd pointer to the command
 */
void pool_broadcast(Command *cmd);

/**
 * @brief Prints info about active pools
 */
void pool_show();

/**
 * @brief Prints finished jobs
 */
void pool_finished();

/**
 * @brief Sends SIGTERM to all active pools
 * @return 1 if no pools are active, 0 otherwise
 */
int pool_shutdown();

/**
 * @brief Updates pool status to exited, cleans up
 * @param pid pool process id
 * @return 1 if no pools are active, 0 otherwise
 */
int pool_exited(pid_t pid);

/**
 * @brief Prints total job count, and job in progress count
 */
void pool_print_info();

/**
 * @brief Prints status of finished jobs & forwards command to pools
 * @param cmd pointer to the command
 */
void pool_status_all(Command *cmd);

/**
 * @brief Prints status of job by id
 *
 * If id is not found on finished jobs, command
 * gets broadcast to pools.
 *
 * @param cmd pointer to the command
 */
void pool_status(Command *cmd);

#endif