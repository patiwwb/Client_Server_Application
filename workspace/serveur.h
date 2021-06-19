#include "pse.h"

#define MAX_CLIENTS 100
#define BUFFER_SZ 2048
#define NOM_JOURNAL   "journal.log"
#define SIZE 1024

static _Atomic unsigned int cli_count = 0;
static int uid = 10;
int fdJournal;
static int envoi = 0;
const char * separators = " ";
const char* separators_extension = " .";
char filename_dest[20] = "recv.txt";
//char filename_dest[20] = "../save/recv.txt";
char filename_source[20];
char recv_file[sizeof "../save/file%03dtxt "];
int file_counter = 0;
char extension[20];
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

//Récupérer l'extension du fichier .txt .jpeg .png
void get_file_extension(char *input,const char* separators, char* extension)
{
    int i = 0;
    char * strToken = strtok ( input, separators );
    while ( strToken != NULL ) {
        //printf ( "%s\n", strToken );
        i++;
        strcpy(extension,strToken);
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
void get_pseudo (char* arr1,char* arr2, int length) {
  
  /*
  printf("buffout\n");
  for(int i=0; i <strlen(arr1);i++)
  {
  	printf("%c",arr1[i]);
  }
  printf("\npseudo\n");
  for(int i=0; i <strlen(arr2);i++)
  {
  	printf("%c",arr2[i]);
  }
  printf("\n");
  */
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
void rogner_message (char* arr1,char* arr2, int length) {
  /*
  printf("buffout\n");
  for(int i=0; i <strlen(arr1);i++)
  {
  	printf("%c",arr1[i]);
  }
  */

  int start = 0;
  int i;
  int j = 0;
  for (i = 0; i <= length; i++) { 
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
  /*
   printf("\nmessage\n");
  for(int i=0; i <strlen(arr2);i++)
  {
  	printf("%c",arr2[i]);
  }
  printf("\n");
  */
  
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


//Fonction pour envoyer des fichiers de data 
void envoi_data(char* buff_out)
{
			get_file_extension(filename_source,separators_extension,extension);
			//printf("Fichier de type |%s|\n",extension);
			//printf("|%s|\n",filename_source);
			strcat(filename_source,".");
			strcat(filename_source,extension);
			//printf("|%s|\n",filename_source);

			if(strcmp(extension,"txt")==0)
			{
				FILE *fp_source;
				FILE *fp_dest;
				fp_dest = fopen(filename_dest, "w");
				fp_source = fopen(filename_source,"r");
				if(fp_source == NULL)
				{
					perror("Erreur lecture fichier\n");
					return; //Si pas de fichier source existant on continu (on arrete surtout pas le serveur)
				}
				sprintf(recv_file, "../save/file%03d.txt", file_counter);
				//printf("|%s|\n",recv_file);
				fp_dest = fopen(recv_file, "w");
				if(fp_dest == NULL)
				{
					perror("Erreur écriture fichier\n");
					return;
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
				fclose(fp_dest);
			}
			else if(strcmp(extension,"jpg")==0 || strcmp(extension,"jpeg")==0 )
			{
				char buffer[1];

				FILE* in;
				FILE* out;
				sprintf(recv_file, "../save/file%03d.jpg", file_counter);
				in = fopen(filename_source, "rb");
				out = fopen(recv_file, "wb");
				if (in == NULL) {
					perror("Erreur lecture fichier\n");
					return;
				}

				if (out == NULL) {
					perror("Erreur ecriture fichier\n");
					return;
				}

				size_t elements_read;
				int count = 0;

				while((elements_read = fread(buffer, sizeof(buffer), 1, in)) == 1)
				{
					count ++;
					//printf("%d\n",count);
					size_t elements_written = fwrite(buffer, sizeof(buffer), 1, out);
					if (elements_written == 0) {
						exit(1);
					}

				}


				if (elements_read == 0) {
					//printf("Envoi fichier terminé\n");
					bzero(buff_out, BUFFER_SZ);
					envoi = 0;	
					fclose(in);
					fclose(out);
				}

				/*bzero(buff_out, BUFFER_SZ);
				envoi = 0;	
				fclose(in);
				fclose(out);*/
			}
			
			else if(strcmp(extension,"png")==0)
			{
				char buffer[1];

				FILE* in;
				FILE* out;
				sprintf(recv_file, "../save/file%03d.png", file_counter);
				in = fopen(filename_source, "rb");
				out = fopen(recv_file, "wb");
				if (in == NULL) {
					perror("Erreur lecture fichier\n");
					return;
				}

				if (out == NULL) {
					perror("Erreur ecriture fichier\n");
					return;
				}

				size_t elements_read;
				int count = 0;

				while((elements_read = fread(buffer, sizeof(buffer), 1, in)) == 1)
				{
					count ++;
					//printf("%d\n",count);
					size_t elements_written = fwrite(buffer, sizeof(buffer), 1, out);
					if (elements_written == 0) {
						exit(1);
					}

				}


				if (elements_read == 0) {
					//printf("Envoi fichier terminé\n");
					bzero(buff_out, BUFFER_SZ);
					envoi = 0;	
					fclose(in);
					fclose(out);
				}

				/*bzero(buff_out, BUFFER_SZ);
				envoi = 0;	
				fclose(in);
				fclose(out);*/
			}
			
			else if(strcmp(extension,"wav")==0)
			{
				char buffer[1];

				FILE* in;
				FILE* out;
				sprintf(recv_file, "../save/file%03d.wav", file_counter);
				in = fopen(filename_source, "rb");
				out = fopen(recv_file, "wb");
				if (in == NULL) {
					perror("Erreur lecture fichier\n");
					return;
				}

				if (out == NULL) {
					perror("Erreur ecriture fichier\n");
					return;
				}

				size_t elements_read;
				int count = 0;

				while((elements_read = fread(buffer, sizeof(buffer), 1, in)) == 1)
				{
					count ++;
					//printf("%d\n",count);
					size_t elements_written = fwrite(buffer, sizeof(buffer), 1, out);
					if (elements_written == 0) {
						exit(1);
					}

				}


				if (elements_read == 0) {
					printf("Envoi fichier terminé\n");
					bzero(buff_out, BUFFER_SZ);
					envoi = 0;	
					fclose(in);
					fclose(out);
				}

				/*bzero(buff_out, BUFFER_SZ);
				envoi = 0;	
				fclose(in);
				fclose(out);*/
			}
			else
			{
				printf("Fichier de type inconnu\n");
			}

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
	//FILE *fp_dest;
	//FILE *fp_source;
	//char *filename_dest = "recv.txt";
	//char *filename_source = "send.txt";
	//char buffer[SIZE];
	/*
	fp_dest = fopen(filename_dest, "w");
	//fp_source = fopen(filename_source,"r");
	if(fp_dest == NULL)
	{
		perror("Erreur lecture dest\n");
		exit(1);
	}*/
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
			//printf("Envoi data:|%s|\n",buff_out);
			envoi_data(buff_out);
			envoi = 0;
			//printf("Longueur:%ld\n",strlen(filename_source));
			for(int i =0; i<strlen(filename_source);i++)
			{
				filename_source[i]='\0';
			}
			filename_source[0]='\0';
			for(int i =0; i<strlen(extension);i++)
			{
				extension[i]='\0';
			}
			extension[0]='\0';
			for(int i =0; i<strlen(buff_out);i++)
			{
				buff_out[i]='\0';
			}
			
		}
		if(!envoi)
		{
			//printf("|%s| |%s|\n",filename_source,extension);
			//printf("buff_out(%ld):%s",strlen(buff_out),buff_out);
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
					//printf("avant:|%s|\n",message);
					get_pseudo (buff_out, pseudo, strlen(buff_out));
					rogner_message(buff_out,message,strlen(buff_out));

					if (strncmp(message, " init",5) == 0) {
						printf("\nRemise a zero du journal\n");
						remiseAZeroJournal();

					}
					//printf("après|%s|\n",message);
					if (strncmp(message, " send",5) == 0) {
						printf("Préparation transfert fichier\n");
						//printf("message:|%s|\n",message);
						char *copy = (char*)malloc(sizeof(char)*(strlen(message)+1));
    					strcpy(copy,message);
    					get_file_name(copy,separators,filename_source);
    					//printf("|%s|\n",filename_source);
    					printf("Envoi du fichier %s\n",filename_source);
						envoi = 1;
						strcpy(message,"0000000");//Pour éviter envoi infini eavec entre apres 1er envoi
						//printf("RESET message\n");
						for(int i =0; i<strlen(message);i++)
						{
							message[i]='\0';
						}
						//printf("Message:%s\n",message);
						file_counter++;
						
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
  //fclose(fp_dest);
  //fclose(fp_source);
	return NULL;
}
