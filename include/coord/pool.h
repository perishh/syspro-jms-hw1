#ifndef POOL_H
#define POOL_H

#include "command.h"

int pool_init();
void pool_free();
int pool_redirect(int fd);
int pool_submit(Command *cmd);
void pool_broadcast(Command *cmd);
void pool_show();

#endif