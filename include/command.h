#ifndef COMMAND_H
#define COMMAND_H

typedef enum {
    UNKNOWN = 0,
    SUBMIT = 1,
    STATUS = 2,
    STATUS_ALL = 4,
    SHOW_ACTIVE = 8,
    SHOW_POOLS = 16,
    SHOW_FINISHED = 32,
    SUSPEND = 64,
    RESUME = 128,
    SHUTDOWN = 256
} Action;

#define ZERO_ARG_ACTIONS (SHOW_ACTIVE | SHOW_POOLS | SHOW_FINISHED | SHUTDOWN)

// FAM pattern
typedef struct {
    Action action;
    int data;
    int len;
    char args[];
} Command;

#endif