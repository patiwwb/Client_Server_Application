#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "pse.h"

#define LENGTH 2048
#define SIZE 1024

// Variables globales côté client
volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[20];


void envoie_msg_client() {
  char message[LENGTH] = {};
	char buffer[LENGTH + 32] = {};

  while(1) {
    printf("%s", ">>> ");
    fflush(stdout);
    fgets(message, LENGTH, stdin);
	for (int i = 0; i < LENGTH; i++) { // trim \n
	if (message[i] == '\n') {
		message[i] = '\0';
		break;
	}
	}
    if (strcmp(message, "fin") == 0) {
    		//flag=1;
			break;
    } else {
      sprintf(buffer, "(%s): %s\n", name, message);
      send(sockfd, buffer, strlen(buffer), 0);
    }

		bzero(message, LENGTH);
    bzero(buffer, LENGTH + 32);
  }
  flag=1;
}

void rec_msg_client() {
	char message[LENGTH] = {};
  while (1) {
		int receive = recv(sockfd, message, LENGTH, 0);
    if (receive > 0) {
      printf("%s", message);
      printf("%s", ">>> ");
      fflush(stdout);
    } else if (receive == 0) {
			break;
    } else {
			// -1
		}
		memset(message, 0, sizeof(message));
  }
}
