#ifndef JOB_H
#define JOB_H

#include <sys/types.h>

typedef struct {
  int id;
  pid_t pid;
  int suspended;
  int finished;
  time_t timestamp;
} Job;

int job_init();
void job_free();
int job_add(int id, char *raw);
int job_suspend(int id);
int job_resume(int id);
int job_continued(pid_t pid);
int job_stopped(pid_t pid);
int job_exited(pid_t pid);
void job_show_finished();
int job_status(int id);
void job_status_all(int n);
void job_show_active();
int job_shutdown();

#endif