#ifndef CMD_H
#define CMD_H

#include "command.h"

/**
 * @brief Initialized buffer for command parsing
 * @return 0 on success, -1 otherwise
 */
int cmd_init();

/**
 * @brief Parses command from a file descriptor
 * @param fd File descriptor to read command from
 * @return Pointer to the parsed command on success,
 * NULL otherwise
 */
Command *cmd_read(int fd);

/**
 * @brief Deallocates buffer
 */
void cmd_free();

#endif