//General Stuff
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

//Multithreading stuff
#include <pthread.h>
#include "fdNode.h"

//FD LINKED LIST METHODS:
//These methods maintain a linked list of all the client FD's so we
//know who we are currently talking to.
//This allows us to end all clients when the server receives
//a SIGINT signal.
fdnode* create_node(int fd){
	fdnode* ret = (fdnode *)malloc(sizeof(fdnode));
	ret->fd = fd;
	ret->next = NULL;
	return ret;
}

fdnode* add(int fd, fdnode * head){
	fdnode* new_node = create_node(fd);
	if(head == NULL){
		head = new_node;
		return head;
	}
	fdnode* temp = head->next;
	head->next = new_node;
	new_node->next = temp;
	return new_node;
}

int delete(int fd, fdnode * head){
	fdnode* temp = head;
	fdnode* next;
	if(temp != NULL){
		if(temp->fd == fd){
			head = temp->next;
			free(temp);
			return 0;
		}
		next = temp->next;
	} else {
		return 0;
	}
	while(next != NULL){
		if(next->fd == fd){
			temp->next = next->next;
			fdnode* tempnext = next->next;
			free(next);
			next = tempnext;
		}
	}
	return 0;
}