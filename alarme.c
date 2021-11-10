#include "alarme.h"


void atende(struct Alarme* alarme) {
	alarme->flag = 1;
   alarme->count++;
}


