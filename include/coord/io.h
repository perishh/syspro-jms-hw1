#ifndef IO_H
#define IO_H

extern int JMSIN_FILENO;
extern int JMSOUT_FILENO;

int io_init();
void io_close();
void io_free();

#endif