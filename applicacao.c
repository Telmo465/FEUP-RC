#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "writenoncanonical.h"

int main (int argc, char** argv)  {

    int fd;

    if ( (argc < 2) || 
  	    ((strcmp("/dev/ttyS0", argv[1])!=0) && 
        (strcmp("/dev/ttyS1", argv[1])!=0) && 
        (strcmp("/dev/ttyS10", argv[1])!=0) && 
  	    (strcmp("/dev/ttyS11", argv[1])!=0) )) {
            printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
            exit(1);
    }

 

    if(llopen(fd, argv[1], atoi(argv[2]))) {
        return 1;
    }

    if(llclose(fd)) {
        return 1;
    }
    
    return 0;
}
