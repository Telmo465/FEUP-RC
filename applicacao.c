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




/*
int getFileSize(FILE* file) {

    int size;

    fseek(file, 0L, SEEK_END);
    size = ftell(file);

    fseek(file, 0L, SEEK_SET);

    return size;
} 


int splitFile(FILE* file, char** chunks) {

    char line[255];
    int lineCounter = 1;
    int fileSize = getFileSize(file);

    if(!file) {
        return 1;
    }

    while(fgets(line, sizeof line, file) != NULL) {
        


    }

}
*/
extern int fd_filesize, fd_file, fd_packetSize = 1024, packetsUnsent;

int readFileData(char* file_name){
    struct stat buf;
    int fd = open(file_name, O_RDONLY);
    stat(file_name, &st);
    fd_filesize = buf.st_size;
    fd_file = fd;
}

void createPacket(unsigned char* packet, unsigned char* buffer, int size, int packetsSent){
    packet[0] = 0x01;
    packet[1] = packetsSent % 255;
    packet[2] = size / 256;
    packet[3] = size % 256;

    for (int x = 0; x < fd_packetSize; x++){
        packet[4 + x] = buffer[x];
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
            printf("Error reading\n")
        }
        index++;
        unsigned char packet[4 + fd_packetSize];
        createPacket(packet, buffer, size, packetsSent);
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

    strcpy(file_name, argv[2]);

    switch (flag_name) {
        case TRANSMITTER:
            readFileData(file_name);
            if(sendDataPacket(fd)) {
                return 1;
            }
            break;
        case RECEIVER:
            char buf[255];
            int packetSize = 0, aux;
            int file_size = 144;//Using control packet file size


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
   

    

    if(llclose(fd)) {
        return 1;
    }
    
    return 0;
}
