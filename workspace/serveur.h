#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
#include "pse.h"
#include <time.h>

#define MAX_CLIENTS 100
#define BUFFER_SZ 2048
#define NOM_JOURNAL   "journal.log"
#define SIZE 1024

static _Atomic unsigned int cli_count = 0;
static int uid = 10;
int fdJournal;
static int envoi = 0;
const char * separators = " ";
char filename_dest[20] = "recv.txt";
//char filename_dest[20] = "../save/recv.txt";
char filename_source[20];
void remiseAZeroJournal(void);

// Recuperer le nom du fichier à transférer
void get_file_name(char* input,const char* separators,char* filename)
{

    // On cherche à récupérer, un à un, tous les mots (token) de la phrase
    // et on commence par le premier.
    int i = 0;
    char * strToken = strtok ( input, separators );
    while ( strToken != NULL ) {
        //printf ( "%s\n", strToken );
        i++;
        strcpy(filename,strToken);
        // On demande le token suivant.
        strToken = strtok ( NULL, separators );
    }
    
}

/* Structure d'un client*/
typedef struct{
	struct sockaddr_in address;
	int sockfd;
	int uid;
	char name[32];

} Client;

Client *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// Afficher heure locale
void print_heure(void)
{
	time_t current_time;
	time(&current_time);
	char heure[14];
	strncpy(heure,&ctime(&current_time)[11],8);
	printf("%s", heure);
}

// Recuperer le pseudo du client
void *get_pseudo (char* arr1,char* arr2, int length) {
  
  int i;
  for (i = 0; i < length; i++) { 
    if (arr1[i] == ':') {
      arr2[i] = '\0';
      break;
    }
    else 
    { 
    	arr2[i]=arr1[i];
    }
  }
  
}

/* Enlever les \n à la fin d'un message pour affichage correct */
void *rogner_message (char* arr1,char* arr2, int length) {
  int start = 0;
  int i;
  int j = 0;
  for (i = 0; i < length; i++) { 
    if(start)
    {	
    	//printf("%c %i\n",arr1[i],i);
    	if (arr1[i] == '\n') {
      		arr2[j] = '\0';
      		j++;
     	 	break;
     	}
     	else
     	{
    		arr2[j]=arr1[i];
    		j++;
  
    	}
    }
    if (arr1[i] == ':') {
      start=1;
    }
  }
  
}


/* Ajouter client dans la file*/
void queue_push(Client *cl){
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i){
		if(!clients[i]){
			clients[i] = cl;
			break;
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* Retirer client de la file*/
void queue_pop(int uid){
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i){
		if(clients[i]){
			if(clients[i]->uid == uid){
				clients[i] = NULL;
				break;
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* Envoi message à tous sauf à la source*/
void envoie_msg_all(char *s, int uid){
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i<MAX_CLIENTS; ++i){
		if(clients[i]){
			if(clients[i]->uid != uid){
				if(write(clients[i]->sockfd, s, strlen(s)) < 0){
					perror("ERREUR: écriture all échec");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

/* Communication avec client */
void *com_client(void *arg){
	char buff_out[BUFFER_SZ];
	char name[32];
	int leave_flag = 0;
	char journal_log[LIGNE_MAX];
	int lgEcr;

	cli_count++;
	Client *cli = (Client *)arg;
	
	//
	FILE *fp_dest;
	//FILE *fp_source;
	//char *filename_dest = "recv.txt";
	//char *filename_source = "send.txt";
	//char buffer[SIZE];
	fp_dest = fopen(filename_dest, "w");
	//fp_source = fopen(filename_source,"r");
	if(fp_dest == NULL)
	{
		perror("Erreur lecture dest\n");
		exit(1);
	}
	/*
	if(fp_source == NULL)
	{
		perror("Erreur lecture source\n");
		exit(1);
	}
	*/
	// Name
	if(recv(cli->sockfd, name, 32, 0) <= 0 || strlen(name) <  2 || strlen(name) >= 32-1){
		printf("Erreur écriture login.\n");
		leave_flag = 1;
	} else{
		strcpy(cli->name, name);
		sprintf(buff_out, "%s a rejoint la session(%s:%hu)\n", cli->name,stringIP(ntohl(cli->address.sin_addr.s_addr)), ntohs(cli->address.sin_port));
		printf("%s", buff_out);
		envoie_msg_all(buff_out, cli->uid);
	}

	bzero(buff_out, BUFFER_SZ);
	while(1){
		if (leave_flag) {
			break;
		}

		int receive = recv(cli->sockfd, buff_out, BUFFER_SZ, 0);
		if (receive == -1)
			erreur_IO("lecture ligne");

		if (receive == 0) { // connexion fermee  => arret brutal du client
			printf("serveur: arret du client\n");
			leave_flag = 1;
		}
		if(envoi)
		{	
			FILE *fp_source;
			fp_source = fopen(filename_source,"r");
			if(fp_source == NULL)
			{
				perror("Erreur lecture source\n");
				exit(1);
			}
			//sending files
			printf("Envoi fichier terminé\n");
			//printf("\n!%s!\n",buff_out);
			char data[SIZE] = {0};
			while(fgets(data, SIZE, fp_source) != NULL) {
				//printf("\n%s\n",data);
				fprintf(fp_dest, "%s", data);
			}
			bzero(data, SIZE);
			bzero(buff_out, BUFFER_SZ);
			envoi = 0;	
			fclose(fp_source);
		}
		if(!envoi)
		{
		
			//printf("buff_out:%s",buff_out);
			if (strcmp(buff_out, "init") == 0) {
						printf("Remise a zero du journal\n");
						remiseAZeroJournal();
			}
			if (receive > 0){
				if(strlen(buff_out) > 0){
					envoie_msg_all(buff_out, cli->uid);
					for (int i = 0; i < strlen(buff_out); i++) { // trim \n
						if (buff_out[i] == '\n') {
							buff_out[i] = '\0';
							break;
						}
					}
					char pseudo[30];
					char message[64];
					get_pseudo (buff_out, pseudo, strlen(buff_out));
					rogner_message(buff_out,message,strlen(buff_out));

					if (strncmp(message, " init",5) == 0) {
						printf("\nRemise a zero du journal\n");
						remiseAZeroJournal();

					}
					if (strncmp(message, " send",5) == 0) {
						printf("\nPréparation envoie fichier\n");
						//printf("message:|%s|\n",message);
						char *copy = (char*)malloc(sizeof(char)*(strlen(message)+1));
    					strcpy(copy,message);
    					get_file_name(copy,separators,filename_source);
    					printf("Envoi du fichier %s\n",filename_source);
						envoi = 1;
					}

					printf("%s : %s:%hu ",pseudo,stringIP(ntohl(cli->address.sin_addr.s_addr)), ntohs(cli->address.sin_port));
					//printf("%s -> %s ", buff_out, cli->name);
					strcpy(journal_log,buff_out);
					strcat(journal_log,"->");
					strcat(journal_log,cli->name);
					time_t current_time;
					time(&current_time);
					char heure[14];
					strncpy(heure,&ctime(&current_time)[11],8);
					strcat(journal_log," (");
					strcat(journal_log,heure);
					strcat(journal_log,") ");
					//printf("log : %s\n",journal_log);
					lgEcr = ecrireLigne(fdJournal, journal_log);
					if (lgEcr == -1)
		    			erreur_IO("ecriture journal");
		    		//printf("%s",buff_out);
		  		    printf("a envoyé %d octets à ",lgEcr);
		  		    print_heure();
		  			printf("\n");
				}
			} else if (receive == 0 || strcmp(buff_out, "fin") == 0){
				sprintf(buff_out, "%s a quitté la session(%s:%hu)\n", cli->name,stringIP(ntohl(cli->address.sin_addr.s_addr)), ntohs(cli->address.sin_port));
				printf("%s", buff_out);
				envoie_msg_all(buff_out, cli->uid);
				leave_flag = 1;
			} 
			 else if (strcmp(buff_out, "init") == 0) {
				printf("Remise a zero du journal\n");
				remiseAZeroJournal();
			}
			else {
				printf("ERREUR: -1\n");
				leave_flag = 1;
			}

			bzero(buff_out, BUFFER_SZ);
		}
	}
  /* Retirer client de la file et gérer thread*/
  close(cli->sockfd);
  queue_pop(cli->uid);
  free(cli);
  cli_count--;
  pthread_detach(pthread_self());
  fclose(fp_dest);
  //fclose(fp_source);
	return NULL;
}
