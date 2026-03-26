#ifndef GLOBALS_H
#define GLOBALS_H

// chmod(2)
// TODO: Recheck (maybe just user?)
#define MODE_RW (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

extern int jobs_pool;

#endif