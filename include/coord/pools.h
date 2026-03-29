#ifndef POOLS_H
#define POOLS_H

int pools_init();
int pools_enqueue(int len, char *args);
void pools_free();

#endif