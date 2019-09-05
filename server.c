//General Stuff
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

//Signal stuff
#include <errno.h>
#include <signal.h>

//For socket stuff
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

//Multithreading stuff
#include <pthread.h>
#include <semaphore.h>

//#include "accounts.h"
#include "accounts.c"
#include "fdNode.c"
#define PORT "9572"
#define DATA_LIM 1024


//Global variables
account** bank;
int numAccounts;
pthread_mutex_t accountLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t account_mutexes[5000];
fdnode* head = NULL;
volatile sig_atomic_t flag = false;
sem_t bankSem;

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//Method for a new account to be added to bank
void createAccount(char * name, char buffer[])
{
  
  //First check if the Bank data is being printed.
  //During this time, no new accounts can be created
  if(flag)
  {
    sprintf(buffer, "Account information is currently being printed. Cannot create account, try again.\n");
    return;      
  }
  //memset(buffer, '\0', strlen(buffer));
  //Need to check if the account name already exists and if an account is inSession
  if(name == NULL)
  {
    sprintf(buffer, "Error account name not specified (createAccount function)\n");
    return;
  }
  
  int i = 0;
  for(i = 0; i < 5000; i++)
  {
    if(bank[i] == NULL)
    {
      continue;
    }
    if(strcmp(bank[i]->name, name) == 0)
    {
      sprintf(buffer, "Account name exists please pick another name.\n");
      return;
    }
  }  
  
  account * newAccount;
  newAccount = addAccountToBank(bank, name);
  //printf("Created new account with name: %s \n", bank[numAccounts]->name);
  numAccounts++;
  
  sprintf(buffer, "Created new account with name: %s\n", newAccount->name);
}

//Method to allow service an account
int serveAccount(char * name, char buffer[])
{
  //Need to set inSession flag of an account to 1
  account * acc;
  acc = getAccount(bank, name); //Find the account in the bank
  
  //printf("reached serveAccount method with name: %s\n", name);
  
  if(name == NULL)
  {
    sprintf(buffer, "Account name not specified\n");
    return -1;
  }
  
  int index = getIndex(bank, name);
  
  if(acc == NULL) //No account found with this name
  {
    sprintf(buffer, "No account found with this name (serve command).\n");
    return -1;
  }
  else if(acc != NULL && pthread_mutex_trylock(&account_mutexes[index]) != 0)
  {
    sprintf(buffer, "ERROR: Account is already in session.\n");
    return 0;
  }
  else
  {
    acc->inSession = true;
    sprintf(buffer, "Account %s is now in service.\n", acc->name);
    return 1;
  }
  return -1;
}

void depositToAccount(char * accountName, char * info, char buffer[]) //info variable hold amount to deposit
{
  if(accountName == NULL || (strcmp(accountName, "")) == 0) //No accountName found. Means account was not created for this client
  {
    sprintf(buffer, "Cannot deposit because account not specified\n");
  }
  else //Account name specificed
  {
    account * acc;
    acc = getAccount(bank, accountName); //Find the account in the bank
    
    //printf("in depositToAccount, account name is: %s\n", acc->name);
    
    if(acc == NULL) //Account not found
    {
      sprintf(buffer, "No account found with this name (depostToAccount method)\n");
    }
    else //Account found
    {
      //First check is the inSession flag is set to 1
      if(acc->inSession == true) //Then allow to deposit
      {
        //Convert "information" to a double
        double depositAmount;
        depositAmount = strtod(info, NULL);
        if(depositAmount < 0) //Deposit amount cannot be negative
        {
          sprintf(buffer, "Cannot deposit a negative amount\n");
          return;
        }
        else
        {
          acc->balance = acc->balance + depositAmount;
          sprintf(buffer, "Balance of account %s is %lf\n", acc->name, acc->balance);
        }
        
      }
      else
      {
        sprintf(buffer, "Account %s is not in session\n", acc->name);
      }
    }
  }
}

void withdrawFromAccount(char * accountName, char * info, char buffer[]) //info variable hold amount to withdraw
{
  if(accountName == NULL || (strcmp(accountName, "")) == 0) //No accountName found. Means account was not created for this client
  {
    sprintf(buffer, "Cannot withdraw because account not specified\n");
  }
  else //Account name specificed
  {
    account * acc;
    acc = getAccount(bank, accountName); //Find the account in the bank
      
    if(acc == NULL) //Account not found
    {
      sprintf(buffer, "No account found with this name (depostToAccount method)\n");
    }
    else //Account found
    {
      //First check is the inSession flag is set to 1
      if(acc->inSession == true) //Then allow to deposit
      {
        //Convert "information" to a double
        double withdrawAmount;
        withdrawAmount = strtod(info, NULL);
        
        if(withdrawAmount < 0)
        {
          sprintf(buffer, "Cannot withdraw a negative amount\n");
          return;
        }
        //Check if the amount they withdraw is more than balance
        if(withdrawAmount > acc->balance)
        {
          sprintf(buffer, "Cannot withdraw more than balance\n");
        }
        else
        {
          acc->balance = acc->balance - withdrawAmount;
          sprintf(buffer, "Balance of account %s is %lf\n", acc->name, acc->balance);
        }
        
      }
      else
      {
        sprintf(buffer, "Account %s is not session\n", acc->name);
      }
    }
  }
}

void query(char * accountName, char buffer[]) //Simply outputs the current balance of an account
{
  account * acc;
  acc = getAccount(bank, accountName); //Find the account in the bank
      
  if(acc == NULL) //Account not found
  {
    sprintf(buffer, "No account found with this name (query method)\n");
  }
  else //Account found
  {
    //Print out current balance
    sprintf(buffer, "Current balance is: %lf\n", acc->balance);
  }
}

void endService(char * accountName, char buffer[])
{
  account * acc;
  acc = getAccount(bank, accountName);
  
  if(acc == NULL) //Account not found
  {
    sprintf(buffer, "No account found with this name (endSerive method).\n");
  }
  else //Account found
  {
    int index = getIndex(bank, accountName);
		pthread_mutex_unlock(&account_mutexes[index]);
   
    acc->inSession = false; //Change inSession flag to 0
    sprintf(buffer, "Ended service to %s.", accountName);
  }
}

void handleCommand(char * buffer, char * accountName, char resultBuffer[])
{
  //printf("account name passed in to handleCommand: %s \n", accountName);
  char fullCommand[DATA_LIM];
  strcpy(fullCommand, buffer); //Copy the text into new variable
  
  char * action;
  action = strtok(fullCommand, " ");
  
  //printf("Action is: %s \n", action);
  
  if(accountName == NULL || accountName == "") //No account name given
  {
    sprintf(resultBuffer, "Cannot handle command because account is not specified\n");
    return;
  }
  char * information;
  information = strtok(NULL, " ");
  
  if(information == NULL || (strcmp(information, "") == 0)) //Then it is a one word command
  {
    
    if((strcmp(action, "create") == 0) || (strcmp(action, "serve") == 0) 
      || (strcmp(action, "deposit") == 0) || (strcmp(action, "withdraw") == 0))
    {
      sprintf(resultBuffer, "Invalid command recieved \n");
    }
    else //ONE WORD COMMAND
    {
      if(strcmp(action, "query") == 0)
      {
        query(accountName, resultBuffer);
      }
      else
      {
        sprintf(resultBuffer, "Invalid command recieved\n");
      }
    }
  }
  else //TWO WORD COMMANDS
  {
    if((strcmp(action, "deposit")) == 0)
    {
      
      //Recognized deposit command
      //Here, the variable "information" hold the amount to deposit
      pthread_mutex_lock(&accountLock);
      //printf("about to call depositToAccount function: %s, %s\n", accountName, information);
      depositToAccount(accountName, information, resultBuffer);
      pthread_mutex_unlock(&accountLock);
      
    }
    else if((strcmp(action, "withdraw")) == 0)
    {
      //Recognized withdraw command
      //Here, the variable "information" hold the amount to withdraw
      pthread_mutex_lock(&accountLock);
      withdrawFromAccount(accountName, information, resultBuffer);
      pthread_mutex_unlock(&accountLock);
    }
  }
}

void* clientServiceThreadFcn(void* arg)
{
  int fd = *(int *)arg;
  add(fd, head); //***Adding the client to a global linked list of clients***
  printf("Connected to client. (fd is: %d)\n", fd);
  //Need a mutex for this??
   
  char * acctName;
  acctName = (char *) malloc(1024);
  int inSessionClient = 0; //Variable to see if a client is in session for an account
  
  //Loop to speak with client and recieve commands
  while(1)
  {
    
    int recvInfo;
    char buffer[DATA_LIM];
    char resultBuffer[DATA_LIM];
    
    if((recvInfo = recv(fd, buffer, 512, 0)) == -1)
    {
      printf("No longer able to recieve data from client\n");
      break;
    }
    else if(recvInfo == 0)
    {
      printf("Connection closed with client\n");
      break;
    }
    
    buffer[recvInfo] = '\0';
    //printf("Recieved from client: %s", buffer);
    
    //Handle the create and quit commands here
    //Assign acctName properly based on create and quit commands
    
    char fullCommand[DATA_LIM];
    strcpy(fullCommand, buffer); //Copy the text into new variable
  
    char * action;
    action = strtok(fullCommand, " ");
  
    char * information;
    information = (char *) malloc(1024);
    information = strtok(NULL, " ");
    
    if((strcmp(action, "create") == 0)) //Handling create command here
    {
      
      //printf("Action is: %s \n", action);
      //Recognized create command
      //Name of account is stored in "information" variable
      
      //Check if the client is already in session for another account
      if(inSessionClient == 1)
      {
       sprintf(resultBuffer, "Already in session for account %s. Cannot create new account\n", acctName); 
      }
      else
      { 
        pthread_mutex_lock(&accountLock);
        createAccount(information, resultBuffer);
        acctName = strdup(information);
        //strcpy(acctName, information);
        pthread_mutex_unlock(&accountLock);
      }
      
    }
    else if ((strcmp(action, "serve")) == 0) //Need to handle serve command here as well
    {
      if(inSessionClient == 1) //Means the client already has an account in session
      {
        sprintf(resultBuffer, "Already in session for account %s. Cannot open another session\n", acctName);
        //printf("Already in session for account %s. Cannot open another session\n", acctName);
      }
      else
      {
        int serveResult = 0;
        //Recognized serve command
        //Name of account is stored in "information" variable
        pthread_mutex_lock(&accountLock);
      
        serveResult = serveAccount(information, resultBuffer);
        //printf("serveResult %d and information is: %s\n", serveResult, information);
        if(serveResult == 0) //Means account is already in session
        {
          acctName = NULL;
        }
        else if(serveResult == -1) //Means no account found with the name given
        {
          acctName = NULL;
        }
        else if(serveResult == 1)
        {
          //printf("Information variable: %s\n", information);
          
          acctName = strdup(information); //Only copy name into acctName if the serveResult is 1
          inSessionClient = 1;
        }
      
        pthread_mutex_unlock(&accountLock);
      }
    }
    else if((strcmp(action, "end") == 0)) //Handling end command as well
    {
      //printf("Action is: %s \n", action);
      //Recognized end command
      //End service to current account
      pthread_mutex_lock(&accountLock);
      endService(acctName, resultBuffer); //Changes inSession flag to 0
      acctName[0] = '\0'; //Reset value of acctName. So create command must be used again
      inSessionClient = 0;
      pthread_mutex_unlock(&accountLock);
    }
    else if((strcmp(action, "quit")) == 0) //End client side
    {
      if(inSessionClient == 1) //Means the user is inSession for an account
      {
        sprintf(resultBuffer, "Please end session for account: %s before quitting.\n", acctName);
        
      }
      else
      {
        printf("Ending client service thread (fd is: %d)\n", fd);
        delete(fd, head); //***Deleteing client from global list***
        close(fd);
        return NULL;
      }
    }
    else
    {
      //printf("acctName variable is %s \n", acctName);
      //Send the revieved text to a function to perform the command
      if(inSessionClient == 1) //Only handle commands if the client is in session
      {
        handleCommand(buffer, acctName, resultBuffer);
      }
      else
      {
        sprintf(resultBuffer, "Cannot handle command\n");
      }
    }
    
    //Send data back to client
    if ((send(fd, resultBuffer, 100,0))== -1) 
    {
			printf("Connection with client ended.\n");
			close(fd);
      break;
		}
  }
  
}

//Accepts incoming client connections from separate client processes
void * sessionAcceptorThreadFcn(void * args)
{
  struct addrinfo hints;
	struct addrinfo * servinfo;
  struct addrinfo * p;
	int socketNum, new_fd;
	char messageToPrint[256] = "";
	char s[INET6_ADDRSTRLEN];
 
  hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	hints.ai_addrlen = 0;
	hints.ai_addr = NULL;
	hints.ai_canonname = NULL;
	hints.ai_next = NULL;
 
  if(getaddrinfo(NULL, PORT, &hints, &servinfo) != 0)
  {
    fprintf(stderr, "Error in getaddrinfo. Unable to connect. \n");
    return;
  } 
  
  //Checking all nodes and binding
  for(p = servinfo; p != NULL; p = p->ai_next)
  {
    socketNum = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if(socketNum == -1)
    {
      fprintf(stderr, "The socket() operation failed \n");
      continue;
    }
    
    if (setsockopt(socketNum, SOL_SOCKET, SO_REUSEADDR, &socketNum,
          sizeof(socketNum)) == -1)
    //(setsockopt(socketNum, SOL_SOCKET, SO_LINGER, &socketNum, sizeof(socketNum)) == -1)
    //CHANGE TO THIS ^
    {
      fprintf(stderr, "setsockopt() function failed \n");
      return;
    }
    if (bind(socketNum, p->ai_addr, p->ai_addrlen) == -1)
    {
      fprintf(stderr, "bind() function failed. Cannot bind the port \n");
      return;
    }
    
    break;
  }
  
  freeaddrinfo(servinfo);
  
  if (p == NULL)  
  {
        fprintf(stderr, "Failed to bind\n");
        exit(1);
  }

  if (listen(socketNum, 10) == -1) 
  {
      fprintf(stderr, "Error with listen() function \n");
      return;
  }
  
  printf("Waiting for connections\n");
  
  struct sockaddr_storage their_addr; // connector's address information
  socklen_t sin_size;
    
  while(1) // main loop for accept()
  {  
        sin_size = sizeof (their_addr);
        new_fd = accept(socketNum, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            fprintf(stderr, "Error with accept() function");
            continue;
        }
        
        //For outputting which client we are connected to
        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        //printf("Got connection from %s\n", s);
        
        //Spawn new client-service thread for each new client
        pthread_t clientService; 
		
		    if(pthread_create(&clientService, 0, 
          clientServiceThreadFcn, &new_fd) != 0)
        {
			    fprintf(stderr, "Unable to create client-service thread.\n");
			    sleep(1);
		    }		
  }
}

void sigintHandler(int sig)
{
  printf("\nServer shutting down. Closing all clients.\n");
	fdnode * temp = head;
  
  while(temp != NULL)
  {
		int fdInt = temp->fd;
		if ((send(fdInt, "Server shutting down. Ending client", 100, 0))== -1) 
    {
			printf("ERROR: Could not shut down one of the clients.\n");
			close(fdInt);
      break;
		}
		temp = temp->next;
	}
	exit(0);
}

void printBank()
{
  printf("\nALL ACCOUNTS:\n");
  
  int i = 0;
  for(i = 0; i < 5000; i++)
  {
    char inSessionString[20];
    strcpy(inSessionString, " ");
    
    if(bank[i] == NULL)
    {
      break;
    }
    else
    {
      if(bank[i]->inSession == 1)
      {
        strcpy(inSessionString, "IN SESSION");
      }
      printf("%s \t %lf \t %s\n", bank[i]->name, bank[i]->balance, inSessionString);
    }
  }
}

void printThreadFcn(void * args)
{
  alarm(15);
  
  while(1)
  {
    sem_wait(&bankSem);
    if(flag)
    {
      printBank();
      flag = false;
      alarm(15);
    }
    sem_post(&bankSem);
  }
}


void handleAlarm(int sig)
{
  flag = true;
}

int main(int argc, char const *argv[])
{
  char * constPortNum = "9572";
  char * portNum = argv[1]; //Port number must be second parameter 
  if(strcmp(portNum, constPortNum) != 0) //portNum entered must equal the predefined one
  {  
    fprintf(stderr, "The port number does not match the one chosen to connect to \n");
    exit(1);
  }
  
  sem_init(&bankSem, 0, 1);
  //Initialize bank
  bank = (account **) malloc(5000 * sizeof(account*));
  numAccounts = 0;
  
  signal(SIGINT, sigintHandler);
  signal(SIGALRM, handleAlarm);
  
  pthread_t sessionAcceptorThread; //Spawn a thread single session-acceptor thread
  if((pthread_create(&sessionAcceptorThread, 0, sessionAcceptorThreadFcn, 0)) != 0)
  {
    fprintf(stderr, "Count not create sessionAcceptorThread\n");
    exit(1);
  }
  
  pthread_t print;
  pthread_create(&print, 0, printThreadFcn, 0);
  
  pthread_join(sessionAcceptorThread, NULL);
  //serverPort(portNum);
  
  sem_destroy(&bankSem);
  
}