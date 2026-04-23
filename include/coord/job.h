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

/**
 * @brief Allocates jobs array
 * @return 0 on success, -1 otherwise
 */
int job_init();

/**
 * @brief Deallocates jobs array
 */
void job_free();

/**
 * @brief Starts new job
 *
 * Checks if there is space available for the job,
 * converts argument string to list, populates job
 * struct on jobs array, forks process, changes its
 * working directory and redirects its outputs to
 * their appropriate files, then calls exec to run
 * the desired program.
 *
 * @param id id of the new job
 * @param raw Raw arguments string from command
 * @return 0 on success, -1 otherwise
 */
int job_add(int id, char *raw);

/**
 * @brief Suspends job by id
 *
 * Finds job by id, checks if already suspended,
 * if not sends SIGSTOP using kill syscall.
 *
 * @param id id of the job to suspend
 * @return 0 on success, -1 otherwise
 */
int job_suspend(int id);

/**
 * @brief Resumes job by id
 *
 * Finds job by id, checks if already running,
 * if not sends SIGCONT using kill syscall.
 *
 * @param id id of the job to resume
 * @return 0 on success, -1 otherwise
 */
int job_resume(int id);

/**
 * @brief Sets job status as running
 * @param pid pid of the job process
 * @return 0 on success, -1 otherwise
 */
int job_continued(pid_t pid);

/**
 * @brief Sets job status as suspended
 * @param pid pid of the job process
 * @return 0 on success, -1 otherwise
 */
int job_stopped(pid_t pid);

/**
 * @brief Sets job status as finished
 *
 * Finds job by pid, updates struct value,
 * sends the job struct to coord so as to
 * preserve after pool exit.
 *
 * @param pid pid of the job process
 * @return 0 on success, -1 otherwise
 */
int job_exited(pid_t pid);

/**
 * @brief Prints finished jobs to coord
 */
void job_show_finished();

/**
 * @brief Shows status of job by id
 * @param id id of the job
 * @return 0 on success, -1 otherwise
 */
int job_status(int id);

/**
 * @brief Prints status of all jobs to coord
 * @param n (optional parameter, 0 if empty) max elapsed seconds
 * since job start
 */
void job_status_all(int n);

/**
 * @brief Prints active jobs to coord
 */
void job_show_active();

/**
 * @brief Sends termination signal to all active jobs
 * @return number of active jobs
 */
int job_shutdown();

/**
 * @return number of jobs given to pool
 */
int get_job_count();

#endif