#ifndef PIPES_H
#define PIPES_H

#define JMS_IN "jms_in"
#define JMS_OUT "jms_out"

int pipes_setup(int epoll_fd);
void pipes_free();

#endif