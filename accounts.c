#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "accounts.h"

//Function to setup an individual account
account * initializeAccount(char * name)
{
  account * newAccount = (account *) malloc(sizeof(account));
  newAccount->name = (char *) malloc(256);
  
  //Set account name to the string passed in
  strcpy(newAccount->name, name);
  
  //Set other fields to default values
  newAccount->balance = 0.0;
  newAccount->inSession = false;
  
  return newAccount;
}

//Function to add the account to the bank struct
account * addAccountToBank(account * bank[], char * name)
{ //Note: "account * bank[]" is an ARRAY OF POINTERS
  
  //Should we have a check for a certain limit of accounts?
  
  account * a = initializeAccount(name); //Initialize individual account
  account ** temp = bank;
  
  int i = 0;
  for(i = 0; i < 5000; i++)
  {
    if(bank[i] == NULL)
    {
      bank[i] = a;
      return a;
    }
    else
    {
      temp++;
    }
  }
  return NULL;
}

//Method to get an account from the bank
account * getAccount(account * bank[], char * accountName)
{
  int j = 0;
  for(j = 0; j < 5000; j++)
  {  
    if(bank[j] == NULL)
    {
      return NULL;
    }
    if(bank[j] != NULL && (strcmp(bank[j]->name, accountName)) == 0)
    {
      return bank[j];
    }
  }
  
  return NULL;
}
int getIndex(account * bank[], char * accountName)
{
  int j = 0;
  for(j = 0; j < 5000; j++)
  {  
    if(bank[j] == NULL)
    {
      return -1;
    }
    if(bank[j] != NULL && (strcmp(bank[j]->name, accountName)) == 0)
    {
      return j;
    }
  }
  
  return -1;
}