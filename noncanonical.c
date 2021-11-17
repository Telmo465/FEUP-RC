/** 
 * Non-Canonical Input Processing
 * From https://tldp.org/HOWTO/Serial-Programming-HOWTO/x115.html by Gary Frerking and Peter Baumann
**/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "auxiliar.h"
#include "constantes.h"

volatile int STOP = FALSE;

void stateMachineUA(enum State* state, char byte) {
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
      else if (byte == C_SET) *state = C_RCV;  // ver valor C
      else *state = START;
      break;
      
    case C_RCV:
      if (byte == BCC1_SET) *state = BCC_OK;
      else if (byte == FLAG) *state = FLAG_RCV;
      else *state = START;
      break;

    case BCC_OK:
      if (byte == FLAG) *state = END;
      else *state = START;
      break;

  }
}

int main(int argc, char **argv) {

  int fd, res;
  struct termios oldtio, newtio;
  unsigned char buf[255];

  if ((argc < 2) ||
      ((strcmp("/dev/ttyS10", argv[1]) != 0) &&
       (strcmp("/dev/ttyS11", argv[1]) != 0) &&
       (strcmp("/dev/ttyS1", argv[1]) != 0) &&
       (strcmp("/dev/ttyS0", argv[1]) != 0) ))
  {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
    exit(1);
  }

  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */

  fd = open(argv[1], O_RDWR | O_NOCTTY);
  if (fd < 0)
  {
    perror(argv[1]);
    exit(-1);
  }

  if (tcgetattr(fd, &oldtio) == -1)
  { /* save current port settings */
    perror("tcgetattr");
    exit(-1);
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
  newtio.c_cc[VMIN] = 0;  /* blocking read until 5 chars received */

  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) pr�ximo(s) caracter(es)
  */

  tcflush(fd, TCIOFLUSH);

  if (tcsetattr(fd, TCSANOW, &newtio) == -1)
  {
    perror("tcsetattr");
    exit(-1);
  }

  printf("New termios structure set\n");

  unsigned char UA[10]; 
  unsigned char Buf[255];

  UA[0] = FLAG;
  UA[1] = A_ER;
  UA[2] = C_UA;
  UA[3] = BCC1_UA;
  UA[4] = FLAG;

  /* 
    O ciclo WHILE deve ser alterado de modo a respeitar o indicado no gui�o 
  */

  int i=0;
  enum State state = START;
  unsigned char byte;

  while(state != END) {
    read(fd, &byte, 1);
    stateMachineUA(&state, byte);
    //Buf[i] = byte;
    i++;
  }

  //Buf[i] = "\0";

  write(fd, UA, 5);
  
  sleep(1);
  tcsetattr(fd, TCSANOW, &oldtio);
  close(fd);
  
  return 0;

}
