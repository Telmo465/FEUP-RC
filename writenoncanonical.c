/*Non-Canonical Input Processing*/

#include "writenoncanonical.h"

volatile int STOP=FALSE;

int count = 0, flag = 1, IWellRecieved = 0;
int numTramaEscrita = 0;
struct termios oldtio,newtio;

int getNumberOfIframe(char* I) {

  char byte;
  int i = 0;
  int num;

  while(byte != FLAG || i > 0) {

    byte = I[i];
    if(byte == C_I0) {
      num = 0
    } else if(byte == C_I1) {
      num = 1
    }else {
      num = -1;
    }
    i++;

  }

  return num;
}

void checkIFrame(char* I) {

  int i=0; 
  char byte;
  bool correct = false;
  
  byte = I[i];

  if(byte != FLAG) {
    return false;
  }

  i++;
  
  while(byte != FLAG) {
    byte = I[i];
    if(i == 1 && byte == A_ER) {
      correct = true;
    }else {
      correct = false;
    }
    if(i == 2 && (byte == C_I0 || byte == C_I1) ) {
      correct = true;
    }else {
      correct = false;
    }
    if(i == 3 && (byte == BCC1_I0 || byte == BCC1_I1)) {
      correct = true;
    }else {
      correct = false;
    }
    if(i == 4 && ()) {

    }else {

    }
  }
}
  

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

//Tanto para o RR como para o REJ
void responseStateMachine(enum State* state, char byte) {

  switch(*state){
    case START:
      if(byte == FLAG) {
        state = FLAG_RCV;
      }
    case FLAG_RCV:
      if(byte == A_RE) {
        state = A_RCV;
      }else if(byte == FLAG) {
        state = FLAG_RCV;
      }else {
        state = START;
      }
    case A_RCV:
      if(byte == C_RR_0 || byte == C_RR_1) {
          state = C_RCV_RR;
      }else if(byte == C_REJ_0 || byte == C_REJ_1) {
        state = C_RCV_REJ;
      }else if(byte == FLAG) {
        state = FLAG_RCV;
      }else {
        state = START;
      }
    case C_RCV_RR:
      if(byte == BCC1_RR_0 || byte == BCC1_RR_1) {
        state = BCC_OK;
      }else if(byte == FLAG) {
        state = FLAG_RCV;
      }else {
        state = START;
      }
      break;
    case C_RCV_REJ:
      if(byte == BCC1_REJ_0 || byte == BCC1_REJ_1) {
        state = BCC_OK;
      }else if(byte == FLAG) {
        state = FLAG_RCV;
      }else {
        state = START;
      }
      break;
    case BCC_OK:
      if(byte == FLAG) {
        state = END;
      }else {
        state = START;
      }
  }
}

void informatioStateMachine(enum State* state, char byte) {

  char data_byte = 0x00;
  int i = 0;

  switch(*state) {
    case START:
      if(byte == FLAG) {
        state = FLAG_RCV;
      }
    case FLAG_RCV:
      if(byte == A_ER) {
        state = A_RCV;
      }else if(byte == FLAG) {
        state = FLAG_RCV;
      }else {
        state = START;
      }
    case A_RCV:
      if(byte == C_I0 || byte == C_I1) {
          state = C_RCV;
      }else if(byte == FLAG) {
        state = FLAG_RCV;
      }else {
        state = START;
      }
    case C_RCV:
      if(byte == FLAG) {
        state = FLAG_RCV;
      }else {
        state = DATA_RCV;
      }
    case DATA_RCV:
      if(byte == data_byte) {
        state = FLAG_RCV2;
      }else {
        data_byte ^= byte;
        i++;
      }
    case FLAG_RCV2:
      state = END;
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

  return i+1;

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
    
  int res;
  char I[2058], byte;
  enum State state = START;
  
  int ISize = buildIFrame(I, buf, length);
  
  
  do {
    printf("##Escreve Informacao##\n");
    write(fd, I, ISize); //Envia o pacote de informacao
  
    if(flag) {
        alarm(3);
        flag = 0;
    }
    
    printf("##Recebe RR ou REJ##");
    
    char response[255];
    int i = 0;   
    while(state != END && flag == 0) { //Recebe RR ou REJ
        read(fd, byte, 1);
        responseStateMachine(state, byte);
        printf("B: %X", byte);//limpar se transmisao para a meio
        respose[i] = byte;
    }
    
    if(response[2] == C_RR_0 || response[2] == C_RR_1) {
        break;
    }
    
  }while(state != END);
  printf("\n");
     
}

int llread(int fd, char* buf) {
  
  int res, i = 0;
  char byte;
  char RR[255], REJ[255];
  enum State state = START;

  printf("Recebe Informacao: \n");
  while(state != END) { //Recebe o pacote de Informacao
    read(fd, byte, 1);
    informatioStateMachine(state, byte); //Corrigir state machine
    I[i] = byte; //Se parar a meio limpar buffer
    printf("B:%X", byte);
    i++;
  }
  printf("\n");    
  
  char I[i];
  
  //Fazer verificacao do trama I
    
  int numFrameI = getNumberOfIframe(I);//numero da frame I recebida
  printf("Num Frame I: %d", numFrameI);  
  
  //Se Trama I bem => RR; Se trama I mal => REJ
  printf("Escreve RR Frame");
  buildRRFrame(RR, numFrameI);
  write(fd, RR, 5); //Envia o pacote RR
  
  /*
  if() {
    buildRRFrame(RR, numFrameI);
    write(fd, RR, 5);
  }else {
    buildREJFrame(fd, REJ);
    write(fd, REJ, 5);
  }
  */
  
  return i+1;
}

int llclose(int fd, int flag_name) {
  unsigned char buf[255];
  
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
  

  //sleep(1);
   
  if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
    perror("tcsetattr");
    return 1;
  }

  close(fd);
  return fd;
}






