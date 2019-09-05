#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct fdnode{
	int fd;
	struct fdnode *next;	
} fdnode;

//fdnode* create_node(int fd);
//fdnode* add(int fd);
//int delete(int fd);
