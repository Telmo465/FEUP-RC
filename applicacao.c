#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "writenoncanonical.h"


int fd_filesize, fd_file, fd_packetSize = 1024, packetsUnsent;


int readFileData(char* file_name){
    struct stat buf;
    int fd = open(file_name, O_RDONLY);
    stat(file_name, &buf);
    fd_filesize = buf.st_size;
    fd_file = fd;
}

void createDataPacket(unsigned char* packet, unsigned char* buffer, int size, int packetsSent){
    packet[0] = 0x01;
    packet[1] = packetsSent % 255;
    packet[2] = size / 256;
    packet[3] = size % 256;

    for (int x = 0; x < fd_packetSize; x++){
        packet[4 + x] = buffer[x];
    }
}

void createControlPacket(unsigned char* packet, char* fileName, int type) {

    if(type = START) {
        packet[0] = START_C;
    }else { 
        packet[0] = END_C;
    }
    
    packet[1] = 0x00; //T1
    packet[2] = sizeof(fd_filesize);
    packet[3] = fd_filesize + '0'; // converts a integer into a char
    
    packet[4] = 0x01; //T2
    packet[5] = sizeof(fileName);
    
    for(int i=0; i<strlen(fileName);i++) {
        packet[i + 6] = fileName[i];
    }
}


int sendDataPacket(int fd){
    int packetsSent = 0, packetsUnsent = fd_file/fd_packetSize;
    unsigned char buffer[fd_packetSize];
    int size = 0;

    if(fd_filesize % fd_packetSize != 0){
        packetsUnsent++;
    }

    int index = 0;
    while(packetsSent < packetsUnsent){
        if((size = read(fd_file,buffer, fd_packetSize)) < 0){
            printf("Error reading\n");
        }
        index++;
        unsigned char packet[4 + fd_packetSize];
        createDataPacket(packet, buffer, size, packetsSent);
        if(llrwrite(fd, packet, 4 + fd_packetSize)) {
            return 1;
        }
        packetsSent++;
    }
}


int main (int argc, char** argv)  {//1 => serial_port; 2 => file_name; 3 => 0 -> TRANSMITTER 1 -> RECEIVER

    int fd = -1;
    char file_name[255];
    int flag_name = atoi(argv[3]);

    if ( (argc < 3) || 
  	    ((strcmp("/dev/ttyS0", argv[1])!=0) && 
        (strcmp("/dev/ttyS1", argv[1])!=0) && 
        (strcmp("/dev/ttyS10", argv[1])!=0) && 
  	    (strcmp("/dev/ttyS11", argv[1])!=0) )) {
            printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
            exit(1);
    }

    if(llopen(&fd, argv[1], flag_name)) {
        return 1;
    }

    printf("LLOPEN\n");

    //strcpy(file_name, argv[2]);
    
    //Writer
    char packet[1024];
    int numPackets = 2, j = 0;

    //Reader
    char buf[255];
    int packetSize = 0, aux;
    int file_size = 1024 * 2;
    
    switch (flag_name) {
        case TRANSMITTER:
            
            while(j < numPackets) {
                for(int i=0; i<1024; i++) {
                packet[i] = 0xFF;
                }
                printf("1.Vou escrever n: %d\n", j);
                if(llrwrite(fd, packet, 1024)) {
                    return 1;
                }
                j++;
            }
            /*
            readFileData(file_name);
            if(sendDataPacket(fd)) {
                return 1;
            }
            */
            break;
        case RECEIVER:
           
            while(packetSize < file_size) {
                if(aux = llread(fd, buf)) {
                    return 1;
                }
            }
            packetSize += aux;
            break;
        default:
            break;
    }
    
    /*Packet so para testar*/
   

    

    if(llclose(fd, flag_name)) {
        return 1;
    }
    
    return 0;
}
