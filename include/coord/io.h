#ifndef IO_H
#define IO_H

extern int JMSIN_FILENO;
extern int JMSOUT_FILENO;

/**
 * @brief Initializes input/output for console communication
 *
 * Deletes previously created pipes (if exist), creates new
 * ones, opens them, adds jms_in to epoll.
 *
 * @return 0 on success, -1 otherwise
 */
int io_init();

/**
 * @brief Closes opened named pipes
 */
void io_close();

/**
 * @brief Deletes names pipes (FIFOs)
 */
void io_free();

#endif