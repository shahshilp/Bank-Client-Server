#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <string.h>


#define PORT "9572" // the port client will be connecting to 
#define MAXDATASIZE 100 // max number of bytes we can get at once 

char sendBuff[1024];
char recvBuff[1024];

void * inputThreadFcn(void * args)
{
  int sockfd = *(int *)args;
  
  while(1)
  {
    printf("Enter command: ");
    
    char command[MAXDATASIZE];
    memset(command, '\0', MAXDATASIZE);
    
    fgets(command, MAXDATASIZE, stdin); //Read stdin input
    strcpy(sendBuff, command); //copy the command into sendBuff
    
    sendBuff[strlen(sendBuff)-1] = '\0'; //Last character is a newLine char so replace with null terminator
    //printf("sendBuff is: %s\n", sendBuff);
    
    if ((send(sockfd,sendBuff, strlen(sendBuff),0)) == -1) //Try to send it to server
    {
      printf("ERROR: Could not send message.\n");
      close(sockfd);
      exit(1);
    }
    
    sleep(2);
  }
  
  return;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void* outputThreadFcn(void* fd){
	int sockfd = *(int *)fd;
	while(1)
  {
    int num = recv(sockfd, recvBuff, sizeof(recvBuff), 0);
    if(num <= 0)
    {
			 printf("Connection closed.\n");
			 return NULL;
    }
		printf("%s\n", recvBuff);
  }
}
 
 
int main(int argc, char *argv[])
{
    int sockfd;
    int numbytes;
    int rv;
      
    char buf[MAXDATASIZE];
    char serverAddress[INET6_ADDRSTRLEN];
    
    struct addrinfo hints;
    struct addrinfo *servinfo;
    struct addrinfo *p;
    
    if (argc != 3) {
        fprintf(stderr,"Invalid format of parameters \n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    //Check if the input port number is the one we chose
    char * constPortNum = "9572";
    char * portNum = argv[2]; //Port number must be third parameter 
    if(strcmp(portNum, constPortNum) != 0) //portNum entered must equal the predefined one
    {  
      fprintf(stderr, "The port number does not match the one chosen to connect to \n");
      exit(1);
    }
  
    char * machineName = argv[1]; //iLab computer will always be second parameter
    if ((rv = getaddrinfo(machineName, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            fprintf(stderr, "Error with socket() function\n");
            continue;
        }
        int numAttempts = 0;
        while(numAttempts < 10) //Tries to connect 100 times
        {
          if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) 
          {
              //close(sockfd);
              //fprintf(stderr, "Error with connect() function\n");
              printf("Could not connect. Trying again...\n");
              sleep(3);
          }
          else
          {
            break;
          }
        }

        break;
    }
    
    
    if (p == NULL) {
        fprintf(stderr, "Client failed to connect\n");
        exit(1);
    }
    
    
    //For outputting which server we are connecting to
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
            serverAddress, sizeof (serverAddress));
    printf("client: connecting to %s\n", serverAddress);
    
    
    freeaddrinfo(servinfo); // done with this struct

	  pthread_t inputThread; //Thread for reading the input commands
    pthread_t outputThread;
     
    if(pthread_create(&inputThread, 0, inputThreadFcn, &sockfd) != 0)
    {
		  fprintf(stderr, "Unable to create command input thread.\n");
		  exit(1);
	  }
    
    if(pthread_create(&outputThread, 0, outputThreadFcn, &sockfd) != 0)
    {
      fprintf(stderr, "Unable to create output thread.\n");
		  exit(1);
    }
    
    pthread_join(inputThread, NULL);
    pthread_join(outputThread, NULL);
    printf("Client end.\n");
    
    return 0;
}