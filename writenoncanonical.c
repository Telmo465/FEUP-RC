/*Non-Canonical Input Processing*/

#include "writenoncanonical.h"

volatile int STOP=FALSE;

int count = 0, flag = 1, IWellRecieved = 0;
int numTramaEscrita = 0;
struct termios oldtio,newtio;

int dataPacketAfterReception(char* I, int length, char* dataPacket) {

  //É preciso fazer destuffing

  int lengthDP = 0;

  for(int i=4; i<length-2; i++) {
    dataPacket[i] = I[i-4];
    lengthDP++;
  }

  return lengthDP;
}

int verifyPacket(char* dataPacket, int length) {

  //É preciso verificar stuffing

  char bcc2 = 0x00;

  for(int i=0; i<length-2; i++) {
    bcc2 ^= dataPacket[i];
  }

  if(bcc2 != dataPacket[length-2]) {
    return 0;
  }

  return 1;
}

//State Machine para I 
void stateMachineInfo(enum State* state, char byte, char* controlByte) {

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
      if (byte == FLAG) {
        *state = FLAG_RCV;
      }
      else if (byte == C_I0 || byte == C_I1) {
        *state = C_RCV;
        *controlByte = byte;
      }
      else *state = START;
      break;
      
    case C_RCV:
      if (byte == BCC1_I1 || byte == BCC1_I0) *state = BCC_OK;
      else if (byte == FLAG) *state = FLAG_RCV;
      else *state = START;
      break;

    case BCC_OK:
      if (byte != FLAG) *state = DATA_RCV;
      else if (byte == FLAG) *state = FLAG_RCV;
      else *state = START;
      break;
    
    case DATA_RCV:
      if (byte == FLAG) *state = END;
      break;

  }
}

//Para o SET and UA
void stateMachineSETAndUA(enum State* state, char byte) {

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
      else if (byte == C_UA || byte == C_SET) *state = C_RCV;
      else *state = START;
      break;
      
    case C_RCV:
      if (byte == BCC1_UA || byte == BCC1_SET) *state = BCC_OK;
      else if (byte == FLAG) *state = FLAG_RCV;
      else *state = START;
      break;

    case BCC_OK:
      if (byte == FLAG) *state = END;
      else *state = START;
      break;
  
  }
}

//Tanto para o RR como para o REJ
void responseStateMachine(enum State* state, char byte, char* controlByte) {

  switch(*state){
    case START:
      if(byte == FLAG) {
        *state = FLAG_RCV;
      }
    case FLAG_RCV:
      if(byte == A_RE) {
        *state = A_RCV;
      }else if(byte == FLAG) {
        *state = FLAG_RCV;
      }else {
        *state = START;
      }
    case A_RCV:
      if(byte == C_RR_0 || byte == C_RR_1) {
        *state = C_RCV_RR;
      }else if(byte == C_REJ_0 || byte == C_REJ_1) {
        *state = C_RCV_REJ;
      }else if(byte == FLAG) {
        *state = FLAG_RCV;
      }else {
        *state = START;
      }
    case C_RCV_RR:
      if(byte == BCC1_RR_0 || byte == BCC1_RR_1) {
        *controlByte = byte; 
        *state = BCC_OK;
      }else if(byte == FLAG) {
        *state = FLAG_RCV;
      }else {
        *state = START;
      }
      break;
    case C_RCV_REJ:
      if(byte == BCC1_REJ_0 || byte == BCC1_REJ_1) {
        *controlByte = byte;
        *state = BCC_OK;
      }else if(byte == FLAG) {
        *state = FLAG_RCV;
      }else {
        *state = START;
      }
      break;
    case BCC_OK:
      if(byte == FLAG) {
        *state = END;
      }else {
        *state = START;
      }
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
    stateMachineSETAndUA(&state, byte);
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
      stateMachineSETAndUA(&state, byte);
    }
    
    if(state == END) {
      stopAlarm();
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
        REJ[2] = C_REJ_0;
        REJ[3] = BCC1_REJ_0;
    }else {
        REJ[2] = C_REJ_1;
        REJ[3] = BCC1_REJ_1;
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

int buildIFrame(char* I, char* buf, int length) {

  char packet[1024];
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

  return i+3;
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

int llrwrite(int fd, char* buf, int length) {
    
  int res, recievedREJ;
  char I[2058], byte;
  enum State state = START;
  
  printf("Started Setup I!\n");

  int ISize = buildIFrame(I, buf, length);
  
  printf("Setup I done!");

  initAlarme();

  do {
    printf("##Escreve Informacao##\n");
    write(fd, I, ISize); //Envia o pacote de informacao
  
    if(flag) {
        alarm(3);
        flag = 0;
    }
    
    
    char controlByte; 
    printf("=============Recebe RR ou REJ=============\n");
    int fr = 0;
    while(state != END && flag == 0) { //Recebe RR ou REJ
      read(fd, byte, 1);
      responseStateMachine(&state, byte, &controlByte);
      if(byte != 0x00) {
        if(fr == 16) {
          printf("\n");
          fr = 0;
        }
        byte = 0x00;
        fr++;
        printf("%X ", byte);//limpar se transmisao parar a meio
      }
    }
    printf("=============Recebe RR ou REJ=============\n");
    
    /*
    if((controlByte == C_RR_0 || controlByte == C_RR_1) && flag == 0) {
      recievedREJ = 0;
      stopAlarm();
      break;
    }else {
      recievedREJ = 1;
      state = START;
    }
    */
  } while(state != END || !recievedREJ);

}

int llread(int fd, char* buf) {
  
  int res, i = 0;
  char byte, controlByte;
  char RR[255], REJ[255], I[2048];
  enum State state = START;

  //printf("========Recebe Informacao=========\n");
  int fr = 0;
  while(state != END) { //Recebe o pacote de Informacao
    read(fd, &byte, 1);
    stateMachineInfo(&state, byte, &controlByte);
    if(byte != 0x00) {
      if(fr == 16) {
        //printf("\n");
        fr = 0;
      }
      //printf("%X ", byte);
      I[i] = byte;
      i++;
      fr++;
    }
  }
  //printf("\n");
  //printf("========Recebe Informacao=========\n");  
  
  
  char dataPacket[2048];
  int lengthDataPacket = dataPacketAfterReception(I, i, dataPacket);
  int numFrameI;

  //Se Trama I bem => RR; Se trama I mal => REJ
  if(verifyPacket(dataPacket, lengthDataPacket)) {
    printf("RR\n");
    if(controlByte == C_I0) {
      numFrameI = 0;
    }else {
      numFrameI = 1;
    }
    buildRRFrame(RR, numFrameI);
    write(fd, RR, 5);
  }else {
    printf("REJ\n");
    if(controlByte == C_I0) {
      numFrameI = 0;
    }else {
      numFrameI = 1;
    }
    buildREJFrame(REJ, numFrameI);
    write(fd, REJ, 5);
  }
  
  return i+1;
}

int llclose(int fd, int flag_name) {
  unsigned char buf[255];
  
  /*
  switch(flag_name){
    case TRANSMITTER:
      
      buf[0] = FLAG;
      buf[1] = A_ER;
      buf[2] = C_DISC;
      buf[3] = BCC_DISC;
      buf[4] = FLAG;
      
      do{
        //send DISC frame
        write(fd,buf,5)
        //start alarm
        //read DISC frame
        int error = readDISC(fd);
        
        if(!error){
          //parar alarme
          //flag alarme 0
          break;
        }
      } while(count <= 3 && flag);
        //stop alarm
      if (count > 3){
        printf("max tries achieved\n");
        return -1;
      }
      //escrever o UA
      buf[0] = FLAG;
      buf[1] = A_ER;
      buf[2] = C_UA;
      buf[3] = BCC1_UA;
      buf[4] = FLAG;
        
      write(fd, buf,5);
      sleep(1);

      break;

    case RECEIVER:
        do{
          //comecar alarme
          //set flag alarme
          int error = readDISC(fd);
            if (!error){
              //start alarme
              //mudar a flag alarme  
              break;
            }
        }while (count <= 3 && flag);

        //stop alarme
        if (count > 3){
        printf("max tries achieved\n");
        return -1;
        }

        buf[0] = FLAG;
        buf[1] = A_ER;
        buf[2] = C_DISC;
        buf[3] = BCC_DISC;
        buf[4] = FLAG;  
        write(fd, buf, 5);
        if (readUA(fd)){
          return 1;
        }
        break;

    default:
      return -1;
  
  */
  //sleep(1);
   
  if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
    perror("tcsetattr");
    return 1;
  }

  close(fd);
  return fd;
}






