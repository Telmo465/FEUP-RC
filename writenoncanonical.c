/*Non-Canonical Input Processing*/

#include "writenoncanonical.h"

volatile int STOP=FALSE;

int count = 0, flag = 1;
int numTramaEscrita = 0;
struct termios oldtio,newtio;

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
      else if (byte == C_SET) *state = C_RCV;  
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

int initStruct(char* serial_port) {

  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */

  int fd = open(serial_port, O_RDWR | O_NOCTTY );
  if (fd <0) {perror(serial_port); exit(-1); }

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

  return fd;
}

void sendUAPacket(int fd) {

  unsigned char UA[10]; 

  UA[0] = FLAG;
  UA[1] = A_ER;
  UA[2] = C_UA;
  UA[3] = BCC1_UA;
  UA[4] = FLAG;

  write(fd, UA, 5);
}

void reciveSETPacket(int fd) {

  enum State state = START;
  unsigned char byte;
  int i=0;

  while(state != END) {
    read(fd, &byte, 1);
    stateMachineUA(&state, byte);
    i++;
  }
}

int sendSETPacket(int fd) {

  char SET[10], byte;
  enum State state = START;
 
  SET[0] = FLAG;
  SET[1] = A_ER;
  SET[2] = C_SET;
  SET[3] = BCC1_SET;
  SET[4] = FLAG;

  initAlarme();

  do {
      
    write(fd, SET, 5);

    if(flag) {
      alarm(3);
      flag = 0;
    }
    
    while(state != END && flag == 0) {
      read(fd, &byte, 1);
      stateMachineSET(&state, byte);
    }
    
    if(state == END) {
      return 0;
    }

    count++;

  } while(count < 3);

  return 1;
}

void buildREJFrame(char* REJ, int numFrameRecieved) {
    
    REJ[0] = FLAG;
    REJ[1] = A_RE;
    
    if(numFrameRecieved == 0) {
        REJ[2] = C_REJ_1;
        REJ[3] = BCC1_REJ_1;
    }else {
        REJ[2] = C_REJ_0;
        REJ[3] = BCC1_REJ_0;
    }
    
    REJ[4] = FLAG;
}

void buildRRFrame(char* RR, int numFrameRecieved) {

    RR[0] = FLAG;
    RR[1] = A_RE;
    
    if(numFrameRecieved == 0) {
        RR[2] = C_RR_1;
        RR[3] = BCC1_RR_1;
    }else {
        RR[2] = C_RR_0;
        RR[3] = BCC1_RR_0;
    }
    
    RR[4] = FLAG;
}

void buildIFrame(char* I, char* buf, int length) {

  char packet[255];
  int i;
   
  I[0] = FLAG;
  I[1] = A_ER;

  if(numTramaEscrita == 0) {
    I[2] = C_I0;
    I[3] = BCC1_I0;
    numTramaEscrita = 1;
  }else {
    I[2] = C_I1;
    I[3] = BCC1_I1;
    numTramaEscrita = 0;   
  }
  
  int j=0;
  for(i=4;i<length+4; i++) {
    I[i] = buf[i-4];
    packet[j] = buf[i-4];
    j++;
  }
  
  I[i] = BCC2(packet, j+1);
  I[i+1] = FLAG;
}

char BCC2(char* buf, int length) {
    
    char byte = 0x00;

    for(int i=0; i<length-1; i++) {
      byte ^= buf[i];
    }

    return byte;
}

int llopen(int* fd, char* serial_port, int flag_name) {

  *fd = initStruct(serial_port);

  switch(flag_name) {
    case TRANSMITTER:
      if(sendSETPacket(*fd)) {
        return 1;
      }
      break;
    case RECEIVER:
      reciveSETPacket(*fd);
      sendUAPacket(*fd);
      break;
    default:
      return 1;
  }
  
  return 0;
}

int llrwrite(int fd, char* buf, int length, int flag_name) {
    
    int res;
    char I[255];
    char RR[255], REJ[255];

    switch (flag_name) {
      case TRANSMITTER:
        //TODO: Fazer timeout
        buildIFrame(I, buf, length);
        res = write(fd,I,length);
        return 0;
        break;
      case RECEIVER:
        //TODO: Se packet recebido bem write RR, se nao foi bem recebido write REJ
        return res;
      default:
        break;
    }

}

int llread(int fd, char* buf) {
  
  char packet[255];

  //TODO:Fazer while para ler packet I recebido com state machine

  return 0;
}

int llclose(int fd) {

  sleep(1);
   
  if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
    perror("tcsetattr");
    return 1;
  }

  close(fd);
  return 0;
}






