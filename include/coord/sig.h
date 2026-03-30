#ifndef SIG_H
#define SIG_H

#include <sys/types.h>

extern int SIG_FILENO;

int sig_init();
void sig_free();

#endif