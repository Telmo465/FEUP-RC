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
int fd_newFile;



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

int createControlPacket(unsigned char* packet, unsigned char* fileName, int type) {

    if(type == 0) {
        packet[0] = START_C;
    }else { 
        packet[0] = END_C;
    }
    
    packet[1] = 0x00;
    packet[2] = sizeof(fd_filesize);
    packet[3] = (fd_filesize >> 24) & 0xFF;
    packet[4] = (fd_filesize >> 16) & 0xFF;
    packet[5] = (fd_filesize >> 8) & 0xFF;
    packet[6] = fd_filesize & 0xFF;

    packet[7] = 0x01;
    packet[8] = strlen(fileName);
    for (unsigned int x = 0; x < strlen(fileName); x++){
        packet[9+x] = fileName[x];
    }

    int packetSize = strlen(fileName) + 9 * sizeof(unsigned char);

    return packetSize;
}


int sendDataPacket(int fd){

    int packetsSent = 0, packetsUnsent = fd_filesize/fd_packetSize;
    unsigned char buffer[fd_packetSize];
    int size = 0;

    if(fd_filesize % fd_packetSize != 0){
        packetsUnsent++;
    }

    unsigned int index = 0;

    while(packetsSent < packetsUnsent){
        if((size = read(fd_file,buffer, fd_packetSize)) < 0){
            return -1;
            printf("Error reading\n");
        }
        index++;
        unsigned char packet[4 + fd_packetSize];
        
        printf("Lol: %d\n", fd_packetSize);

        createDataPacket(packet, buffer, size, packetsSent);
        if(llrwrite(fd, packet, 4 + fd_packetSize)) {
            return -1;
        }
        
        packetsSent++;
    }

    return 0;
}

int getDataForTheFile(unsigned char* dataFile, unsigned char* buf, int size) {

    unsigned int i;
    int sizeDataFile = 256*buf[2] + buf[3];

    printf("Read packet number: %d\n", buf[1]);
    for(i=4; i<size; i++) {
       dataFile[i-4] = buf[i];
    }

    return sizeDataFile;
}


int transmitter(int fd, unsigned char* filename) {

    unsigned char controlPacket[1024];
    int controlPacketSize;

    printf("Created control packet START...\n");
    controlPacketSize = createControlPacket(controlPacket, filename, 0);

    printf("Write control packet START..\n");
    if(llrwrite(fd, controlPacket, controlPacketSize) == -1) {
        return 1;
    }

    printf("Sent data packets...\n");
    if(sendDataPacket(fd) == -1) {
        return 1;
    }
    
    printf("Created control packet END...\n");
    controlPacketSize = createControlPacket(controlPacket, filename, 1);

    
    printf("Write control packet END...\n");
    if(llrwrite(fd, controlPacket, controlPacketSize) == -1) {
        return 1;
    }
}

int reciever(int fd) {

    unsigned char buf[2048];
    int size, fileSize = 0, j, nameFileSize;
    
    printf("Read control packet START...\n");
    //Read start control packet
    if((size = llread(fd, buf)) == -1) {
        return 1;
    }

    printf("Process control packet START...\n");
    for(j=1; j<size; j++) {
        switch(buf[j]) {
            case 0x00:
                j += 2;
                fileSize += (buf[j] << 24);
                j++;
                fileSize += (buf[j] << 16);
                j++;
                fileSize += (buf[j] << 8);
                j++;
                fileSize += buf[j];
                break;
            case 0x01:
                j++;
                nameFileSize = buf[j];
                char* newFileName = (char*) malloc(nameFileSize + 1);
                j++;

                for(int x = 0; x < nameFileSize; x++) {
                    newFileName[x] = buf[j];
                    j++;
                    if(x == nameFileSize-1) {
                        newFileName[j] = '\0';
                        j++; 
                    }
                }

                if((fd_newFile = open(newFileName, O_WRONLY | O_CREAT | O_APPEND, 0664)) == -1) {
                    return 1;
                }
                break;
            default:
                break;
            
        }
    }

    int bytesRead = 0;
    unsigned char dataFile[2048];
    int cont = 0;

    printf("Read File Data...\n");
    //Read file data
    while(1) {
        
        if((size = llread(fd, buf)) == -1) {
            return 1;
        }
        
        printf("size: %d\n", size);

        if(buf[0] == END_C && bytesRead > fileSize) {
            break;
        }

        int fileSizeD = getDataForTheFile(dataFile, buf, size);

        write(fd_newFile, dataFile,fileSizeD);

        bytesRead += size;
        cont++;
    }

    close(fd_newFile);

    return 0;
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
 
    switch (flag_name) {
        case TRANSMITTER:
            strcpy(file_name, argv[2]);
            readFileData(file_name);
            if(transmitter(fd, file_name)) {
                return 1;
            }
            break;
        case RECEIVER:
            if(reciever(fd)) {
                return 1;
            }
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
