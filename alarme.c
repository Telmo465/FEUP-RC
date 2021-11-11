#include "alarme.h"

void initAlarme() {

   (void) signal(SIGALRM, alarme_handler);

   alarme.count = 0;
   alarme.flag = 1;

}

void alarme_handler(int signal) {

   alarme.flag = 1;

}



