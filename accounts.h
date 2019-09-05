#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
  char * name;
  double balance;
  bool inSession;
} account;


account * initializeAccount(char* name);
account * addAccountToBank(account * bank[], char * name);
account * getAccount(account * bank[], char * accountName);