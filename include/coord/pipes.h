#ifndef PIPES_H
#define PIPES_H

#define JMS_IN "jms_in"
#define JMS_OUT "jms_out"

extern int JMSOUT_FILENO;

int pipes_setup();
void pipes_close();
void pipes_free();

#endif