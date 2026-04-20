#ifndef POOL_H
#define POOL_H

#include "command.h"

typedef struct {
  int isText;
  int length;
} PoolMessage;

int pool_init();
void pool_free();
int pool_redirect(int fd);
int pool_submit(Command *cmd);
void pool_broadcast(Command *cmd);
void pool_show();
void pool_finished();
int pool_shutdown();
int pool_exited(int status);
void pool_print_info();
void pool_status_all(Command *cmd);
void pool_status(Command *cmd);

#endif