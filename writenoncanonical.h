
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

int llopen(int* fd, char* serial_port, int flag_name);

void reciveSETPacket(int fd);

void sendUAPacket(int fd);

int sendSETPacket(int fd);

int initStruct(int* fd, char* serial_port);

int llclose(int fd);

#endif