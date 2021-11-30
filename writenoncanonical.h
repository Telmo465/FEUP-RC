
#ifndef WRITENONCANONICAL_H
#define WRITENONCANONICAL_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "constantes.h"
#include "auxiliar.h"

extern int fd_filesize, fd_file, fd_packetSize, packetsUnsent;

int readUA(int fd);
unsigned int dataPacketAfterReception(unsigned char* I, int length, unsigned char* dataPacket);
int verifyPacket(unsigned char* dataPacket, int length, unsigned char bcc2);
void stateMachineInfo(enum State* state, unsigned char byte, unsigned char* controlByte);
void responseStateMachine(enum State* state, unsigned char byte, unsigned char* controlByte);
void stateMachineSETAndUA(enum State* state, unsigned char byte);
int initStruct(unsigned char* serial_port);
void sendUAPacket(int fd);
int reciveSETPacket(int fd);
int sendSETPacket(int fd);
void buildREJFrame(unsigned char* REJ, int numFrameRecieved);
void buildRRFrame(unsigned char* RR, int numFrameRecieved);
int buildIFrame(unsigned char* I, unsigned char* buf, int length);
unsigned char BCC2(unsigned char* buf, int length);
int llopen(int* fd, unsigned char* serial_port, int flag_name);
int llrwrite(int fd, unsigned char* buf, int length);
int llread(int fd, unsigned char* buf);
int llclose(int fd, int flag_name);


#endif