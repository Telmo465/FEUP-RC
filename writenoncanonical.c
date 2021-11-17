/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "auxiliar.h"
#include "constantes.h"

volatile int STOP=FALSE;

int count = 0, flag = 1;

void stateMachineSET(enum State* state, char byte) {

  switch(*state){
    case START:
      if (byte == FLAG) *state = FLAG_RCV;
      break;
      
    case FLAG_RCV:
      if (byte == FLAG) *state = FLAG_RCV;
      else if (byte == A_ER) *state = A_RCV;
      else *state = START;
      break;

    case A_RCV:
      if (byte == FLAG) *state = FLAG_RCV;
      else if (byte == C_UA) *state = C_RCV;  // ver valor C
      else *state = START;
      break;
      
    case C_RCV:
      if (byte == BCC1_UA) *state = BCC_OK;
      else if (byte == FLAG) *state = FLAG_RCV;
      else *state = START;
      break;

    case BCC_OK:
      if (byte == FLAG) *state = END;
      else *state = START;
      break;

  }
}

int main (int argc, char** argv)  {
    
  int fd,c, res;
  struct termios oldtio,newtio;
  char buf[255];
  int i, sum = 0, speed = 0;
    
  if ( (argc < 2) || 
  	    ((strcmp("/dev/ttyS0", argv[1])!=0) && 
        (strcmp("/dev/ttyS1", argv[1])!=0) && 
        (strcmp("/dev/ttyS10", argv[1])!=0) && 
  	    (strcmp("/dev/ttyS11", argv[1])!=0) )) {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
    exit(1);
  }

  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */


  fd = open(argv[1], O_RDWR | O_NOCTTY );
  if (fd <0) {perror(argv[1]); exit(-1); }

  if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
    perror("tcgetattr");
    exit(-1);
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
  newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */

  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) pr�ximo(s) caracter(es)
  */

  tcflush(fd, TCIOFLUSH);

  if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  printf("New termios structure set\n");


  char SET[10];
  char Buf[255];
  char byte;
 
  SET[0] = FLAG;
  SET[1] = A_ER;
  SET[2] = C_SET;
  SET[3] = BCC1_SET;
  SET[4] = FLAG;

  enum State state = START;
  
  initAlarme();

  //sleep(1);

  do {
      
    write(fd, SET, 5);

    if(flag) {
      alarm(3);
      flag = 0;
    }
    
    int i = 0;
    while(state != END && flag == 0) {
      read(fd, &byte, 1);
      //printf("B: %X", byte);
      stateMachineSET(&state, byte);
      //Buf[i] = byte;
      i++;
    }
    
    //Buf[i] = "\0";
    printf("\n");

    if(state == END) {
      break;
    }

    count++;

  }while(count < 3);
  
  /* 
  O ciclo FOR e as instru��es seguintes devem ser alterados de modo a respeitar 
  o indicado no gui�o 
  */

  sleep(1);
   
  if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  close(fd);
  return 0;

}
