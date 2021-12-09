#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define MAX_TIMEOUTS 3

struct dataTransfered {
  unsigned int status; // 0 - Sender || 1 - Receiver
  int ns;
  unsigned int timeouts;
  unsigned int numTries;
  unsigned int alarmFlag;
} data;


void alarm_handler(int signal);
void startAlarm();
void stopAlarm();
