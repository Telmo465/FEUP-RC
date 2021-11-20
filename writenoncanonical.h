
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

void stateMachineUA(enum State* state, char byte);
void stateMachineSET(enum State* state, char byte);
int initStruct(char* serial_port);
void sendUAPacket(int fd);
void reciveSETPacket(int fd);
int sendSETPacket(int fd);
void buildREJFrame(char* REJ, int numT);
void buildRRFrame(char* RR, int numFrameRecieved);
void buildIFrame(char* I, char* buf, int length);
char BCC2(char* buf, int lenght);
int llopen(int* fd, char* serial_port, int flag_name);
int llrwrite(int fd, char* buf, int length, int flag_name);
int llread(int fd, char* buf);
int llclose(int fd);

#endif