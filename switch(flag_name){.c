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
        printf("DISC sent.\n");
        initAlarme(); //start alarm
        flag = 0;
        //read DISC frame
        printf("Waiting for DISC...\n")
        int error = readDISC(fd);
        
        if(!error){
          stopAlarm(); //parar alarme
          flag = 0; //flag alarme 0
          break;
        }
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
            initAlarme();//comecar alarme
            flag = 1;//set flag alarme

            int error = readDISC(fd);
            if (!error){
              stopAlarm();//start alarme
              flag = 0;//mudar a flag alarme  
              break;
            }
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
  

int readUA(int fd) {
    unsigned char received[255];
    read(fd, received, 5);
    printf("Received SET. Checking values...\n");

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
    else if (received[3] != BCC_UA){
        printf("BCC_UA error\n");
        return 1;
    }
    else{
        printf("SET is valid\n");
    }

    return 0;
}