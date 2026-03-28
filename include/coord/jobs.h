#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>

int jobs_submit(char *cmd_args);
int jobs_exited(pid_t pid);
int jobs_stopped(pid_t pid);
int jobs_continued(pid_t pid);
int jobs_suspend(int id);
int jobs_resume(int id);

#endif