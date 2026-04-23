#ifndef ARGS_H
#define ARGS_H

/**
 * @brief Parses path & jobs_pool arguments, changes
 * changes working directory to path.
 *
 * @return 0 on success, -1 otherwise
 */
int args_init(int argc, char **argv);

/**
 * @return jobs_pool value
 */
int get_jobs_pool();

#endif