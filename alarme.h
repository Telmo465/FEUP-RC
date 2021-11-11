#include <string.h>
#include <unistd.h>
#include <signal.h>

struct Alarme {
    int flag;
    int count;
} alarme;

void initAlarme();
void alarme_handler(int signal);



