
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

int dataPacketAfterReception(char* I, int length, char* dataPacket);
int verifyPacket(char* dataPacket, int length);
void stateMachineInfo(enum State* state, char byte, char* controlByte);
void responseStateMachine(enum State* state, char byte, char* controlByte);
void stateMachineSETAndUA(enum State* state, char byte);
int initStruct(char* serial_port);
void sendUAPacket(int fd);
void reciveSETPacket(int fd);
int sendSETPacket(int fd);
void buildREJFrame(char* REJ, int numFrameRecieved);
void buildRRFrame(char* RR, int numFrameRecieved);
int buildIFrame(char* I, char* buf, int length);
char BCC2(char* buf, int length);
int llopen(int* fd, char* serial_port, int flag_name);
int llrwrite(int fd, char* buf, int length);
int llread(int fd, char* buf);
int llclose(int fd, int flag_name);


#endif