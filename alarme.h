#include <unistd.h>
#include <signal.h>


struct Alarme {
    int flag;
    int count;
};

void atende(struct Alarme* alarme);

