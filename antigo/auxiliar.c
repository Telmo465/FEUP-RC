#include "auxiliar.h"

void initAlarme() {
   (void) signal(SIGALRM, alarme_handler);
}

void stopAlarm() {
 
   (void) signal(SIGALRM, NULL);
   alarm(0);
   flag = 1;
}

void alarme_handler(int signal) {

   flag = 1;
}

