#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>

int count_words(const char *args);
int decode_args(char *raw, char **argv);
ssize_t read_blocking(int __fd, void *__buf, size_t __nbytes);

#endif