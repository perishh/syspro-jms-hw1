#ifndef CMD_H
#define CMD_H

#include "command.h"

int cmd_init();
Command *cmd_read(int fd);
void cmd_free();

#endif