#ifndef UTILS_H
#define UTILS_H

#include <sys/types.h>

/**
 * @brief Reads blockingly from a NONBLOCK fd
 *
 * Uses poll to block until data becomes available
 *
 * @param _fd file descriptor to read from
 * @param _buf pointer of buffer to write on
 * @param _nbytes number of bytes to read
 * @return value from read
 */
ssize_t read_blocking(int _fd, void *_buf, size_t _nbytes);

/**
 * @brief Counts words in string
 * @note Delimiters are Space, New Line or NULL
 * @return number of words
 */
int count_words(const char *args);

/**
 * Splits argument string to list
 * @param raw argument string
 * @param argv pointer to the string list
 * @return 0 on success, -1 otherwise
 */
int decode_args(char *raw, char **argv);

#endif