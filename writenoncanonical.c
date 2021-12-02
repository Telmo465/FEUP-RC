/*Non-Canonical Input Processing*/

#include "writenoncanonical.h"

volatile int STOP=FALSE;

int count = 0, flag = 1, IWellRecieved = 0;
int numTramaEscrita = 0;
struct termios oldtio,newtio;

unsigned int dataPacketAfterReception(unsigned char* I, int length, unsigned char* dataPacket) {

  int lengthDP = 0;

  for(unsigned int i=4; i<length-1; i++) {
    dataPacket[i-4] = I[i];
    lengthDP++;
  }

  return lengthDP;
}

void destuffing(unsigned char* dataPacket, int length) {

  unsigned char auxDatapacket[1024];

  unsigned int j=0;
  unsigned int i;

  for(i=0; i<length; i++) {
    if(dataPacket[i] == 0x7D && dataPacket[i+1] == (FLAG ^ 0x20)) {
      auxDatapacket[j] = FLAG;
      i++;
      j++;
    }else if(dataPacket[i] == 0x7D && dataPacket[i+1] == (FLAG ^ 0x20)) {
      auxDatapacket[j] = 0x20;
      i++;
      j++;
    }else {
      auxDatapacket[j] = dataPacket[i];
      j++;
    }
  }

  auxDatapacket[j++] = dataPacket[i++];

  strcpy(dataPacket, auxDatapacket);
}

int verifyPacket(unsigned char* dataPacket, int length) {

  destuffing(dataPacket, length);

  unsigned char bcc2Aux = 0x00;

  for(unsigned int i=0; i<length-1; i++) {
    bcc2Aux ^= dataPacket[i];
  }

  printf("BCC2 %X", bcc2Aux);

  if(bcc2Aux != dataPacket[length-1]) {
    return 0;
  }

  return 1;
}


int readUA(int fd) {
    unsigned char received[255];
    read(fd, received, 5);
    printf("Received SET. Checking values...\n");

    int bccRight;

    if((received[3] == BCC1_UA) == 7) {
      bccRight = 0;
    }else {
      bccRight = 1;
    }
    
    if (received[0] != FLAG || received[4] != FLAG){
        printf("FLAG error\n");
        return 1;
    }
    else if (received[1] != A_ER){
        printf("A_UA error\n");
        return 1;
    }
    else if (received[2] != C_UA){
        printf("C_UA error\n");
        return 1;
    }
    else if (bccRight){
        printf("BCC1_UA error\n");
        return 1;
    }
    else{
        printf("SET is valid\n");
    }

    return 0;
}

int readDISC(int fd) {
    unsigned char received[255];
    read(fd, received, 5);
    printf("Received DISC. Checking values...\n");
    int bccRight;

    if((received[3] != BCC_DISC) == 11) {
      bccRight = 1;
    }else {
      bccRight = 0;
    }

    if (received[0] != FLAG || received[4] != FLAG){
        printf("FLAG error\n");
        return 1;
    }
    else if (received[1] != A_ER){
        printf("A_SET error\n");
        return 1;
    }
    else if (received[2] != C_DISC){
        printf("C_DISC error\n");
        return 1;
    }
    else if (bccRight){
        printf("BCC_DISC error\n");
        return 1;
    }
    else{
        printf("DISC is valid\n");
    }

    return 0;
}


//State Machine para I 
void stateMachineInfo(enum State* state, unsigned char byte, unsigned char* controlByte) {

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
void stateMachineSETAndUA(enum State* state, unsigned char byte) {

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
void responseStateMachine(enum State* state, unsigned char byte, unsigned char* controlByte) {

  switch(*state){
    case START:
      if(byte == FLAG) {
        *state = FLAG_RCV;
      }
      break;
    case FLAG_RCV:
      if(byte == A_RE) {
        *state = A_RCV;
      }else if(byte == FLAG) {
        *state = FLAG_RCV;
      }else {
        *state = START;
      }
      break;
    case A_RCV:
      if(byte == C_RR_0 || byte == C_RR_1) {
        *controlByte = byte; 
        *state = C_RCV_RR;
      }else if(byte == C_REJ_0 || byte == C_REJ_1) {
        *controlByte = byte; 
        *state = C_RCV_REJ;
      }else if(byte == FLAG) {
        *state = FLAG_RCV;
      }else {
        *state = START;
      }
      break;
    case C_RCV_RR:
      if(byte == BCC1_RR_0 || byte == BCC1_RR_1) {
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
      break;
  }
}

int initStruct(unsigned char* serial_port) {

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
  newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

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

int reciveSETPacket(int fd) {

  enum State state = START;
  unsigned char byte;
  int i=0;

  do {

    if(flag){
      alarm(3);
      flag = 0;
    }
    
    while(state != END && flag == 0) {
      read(fd, &byte, 1);
      stateMachineSETAndUA(&state, byte);
      i++;
    }

    if(state == END) {
      count = 0;
      stopAlarm();
      return 0;
    }

    count++;

  }while(count < 3);

  return 1;
}

int sendSETPacket(int fd) {

  unsigned char SET[10], byte;
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
      count = 0;
      stopAlarm();
      return 0;
    }

    count++;

  } while(count < 3);

  return 1;
}

void buildREJFrame(unsigned char* REJ, int numFrameRecieved) {
    
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

void buildRRFrame(unsigned char* RR, int numFrameRecieved) {

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

int buildIFrame(unsigned char* I, unsigned char* buf, int length) {

  unsigned char packet[1024];
  unsigned int i;
   
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
  
  unsigned int j=0;
  for(i=4;i<length+4; i++) {
    I[i] = buf[i-4];
    packet[j] = buf[i-4];
    if (packet[j] == FLAG || packet[j] == 0x7D) {
      I[i] = 0x7D;
      I[i+1] = packet[j] ^ 0x20;
    }else {
      I[i] = buf[i-4];
    }
    j++;
  }
  
  unsigned char bcc2 = BCC2(I, length+4);

  printf("BCC2:%X\n", bcc2);

  I[i] = bcc2;

  
  if (bcc2 == FLAG || bcc2 == 0x7D){
    I[i] = 0x7D;
    I[i + 1] = bcc2 ^ 0x20;
    i++;
  }
  else I[i] = bcc2;
  
  I[i+1] = FLAG;

  return i+2;
}

unsigned char BCC2(unsigned char* buf, int length) {
    
    unsigned char byte = 0x00;

    for(unsigned int i=4; i<length; i++) {
      byte ^= buf[i];
    }

    return byte;
}

int llopen(int* fd, unsigned char* serial_port, int flag_name) {

  *fd = initStruct(serial_port);

  switch(flag_name) {
    case TRANSMITTER:
      if(sendSETPacket(*fd)) {
        return 1;
      }
      break;
    case RECEIVER:
      if(reciveSETPacket(*fd)) {
        return 1;
      }
      sendUAPacket(*fd);
      break;
    default:
      return 1;
  }
  
  return 0;
}

int llrwrite(int fd, unsigned char* buf, int length) {
    
  int res, recievedREJ;
  unsigned char I[2048];
  unsigned char byte;
  enum State state = START;
  
  printf("\nStarted Setup I!\n"); 

  int ISize = buildIFrame(I, buf, length);

  printf("Setup I done!\n");

  initAlarme();

  unsigned int countI = 0;

  do {

    write(fd, I, ISize); //Envia o pacote de informacao

    if(flag) {
        alarm(3);
        flag = 0;
    }
    
    unsigned char controlByte;
    while(state != END && flag == 0) { //Recebe RR ou REJ
      read(fd, &byte, 1);
      responseStateMachine(&state, byte, &controlByte);
    }

    if(flag == 1) {
      countI++;
      continue;
    }

    if((controlByte == C_RR_0 || controlByte == C_RR_1) && state == END) {
      printf("ACEITOU\n");
      stopAlarm();
      return 0;
    }else {
      printf("REJEITOU\n");
      recievedREJ = 1;
      state = START;
      countI++;
    }

    if(countI > 2) {
      break;
    }

  } while(recievedREJ);

  return -1;
}


int llread(int fd, unsigned char* dataPacket) {
  
  int res;
  unsigned char byte, controlByte;
  unsigned char RR[255], REJ[255];
  unsigned char I[2048];
  enum State state = START;

  unsigned int lengthDataPacket = 0;

  int reject = 1, countREJ = 0;

  initAlarme();

  while(1) {
    unsigned int i = 0;
    
    if(countREJ > 2) {
      break;
    }

    if(flag) {
      alarm(3);
      flag = 0;
    }

    while(state != END && flag == 0) { //Recebe o pacote de Informacao
      read(fd, &byte, 1);
      stateMachineInfo(&state, byte, &controlByte);
      I[i] = byte;
      i++;
    }

    if(flag == 1) {
      countREJ++;
      continue;
    }

    lengthDataPacket = dataPacketAfterReception(I, i, dataPacket);

    int numFrameI;
    //Se Trama I bem => RR; Se trama I mal => REJ
    if(verifyPacket(dataPacket, lengthDataPacket)) {
      printf("RR\n");

      printf("size3: %d\n", lengthDataPacket);

      reject = 0;
      if(controlByte == C_I0) {
        numFrameI = 0;
      }else {
        numFrameI = 1;
      }  
      buildRRFrame(RR, numFrameI);
      write(fd, RR, 5);
      return lengthDataPacket-1;
    }else {
      printf("REJ\n");
      state = START;
      if(controlByte == C_I0) {
        numFrameI = 0;
      }else {
        numFrameI = 1;
      }
      buildREJFrame(REJ, numFrameI);
      write(fd, REJ, 5);
      countREJ++;
    }
  }

  return -1;
}

int llclose(int fd, int flag_name) {
  unsigned char buf[255];

  count = 0;

  initAlarme();
  flag = 1;

  switch(flag_name){
    case TRANSMITTER:
      
      buf[0] = FLAG;
      buf[1] = A_ER;
      buf[2] = C_DISC;
      buf[3] = BCC_DISC;
      buf[4] = FLAG;
      
      do{
        //send DISC frame
        write(fd,buf,5);
        printf("DISC sent.\n");
        if(flag) {
          alarm(3);
          flag = 0;
        }
        //read DISC frame
        printf("Waiting for DISC...\n");
        int error = readDISC(fd);
        
        if(!error){
          stopAlarm(); //parar alarme
          flag = 0; //flag alarme 0
          break;
        }
        count++;
        printf("count: %d", count);
      } while(count <= 3 && flag);
      
      stopAlarm(); //stop alarm

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
      printf("UA Sent.\n"); 
      sleep(1);
      break;
    case RECEIVER:
        do{
            if (count >= 1){
              printf("Didn't receive...\n");
            }
            if(flag) {
              alarm(3);
              flag = 0;
            }
            int error = readDISC(fd);
            if (!error){
              stopAlarm();//start alarme
              flag = 0;//mudar a flag alarme  
              break;
            }
            count++;
        }while (count <= 3 && flag);

        stopAlarm();//stop alarme
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
        printf("DISC sent.\n"); 
        
        printf("Waiting for UA...\n");

        if (readUA(fd)){
          return 1;
        }
        break;

    default:
      return -1;
  }
  
  
  sleep(1);
   
  if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
    perror("tcsetattr");
    return 1;
  }

  close(fd);
  return fd;
}






