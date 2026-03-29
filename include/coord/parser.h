#ifndef PARSER_H
#define PARSER_H

#include "command.h"

int parse_arguments(int argc, char **argv, char **path, int *jobs_pool);
int parse_commands(int in, Command** cmd_ptr);

#endif