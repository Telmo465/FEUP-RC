#ifndef ALARME_H
#define ALARME_H


#include <signal.h>

extern int count, flag;

enum State {
  START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, END
};

void initAlarme();
void alarme_handler(int signal);

#endif
