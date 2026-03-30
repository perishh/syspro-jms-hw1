#ifndef POOL_H
#define POOL_H

#include "command.h"

int pool_init();
void pool_free();
int pool_redirect(int fd);
int pool_submit(Command* cmd);

#endif