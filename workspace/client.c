#include "client.h"

void GetClientPseudo(char *name);//Recupere le pseudo écrit par le client 

int main(int argc, char **argv){

	struct sockaddr_in *server_addr;
	if(argc != 3){
		printf("Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}


	signal(SIGPIPE, SIG_IGN);
	
	/* Param socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	server_addr = resolv(argv[1],argv[2]);
	 if (server_addr== NULL)
    	erreur("adresse %s port %s inconnus\n", argv[1], argv[2]);
		
	printf("%s:%hu\n",stringIP(ntohl(server_addr->sin_addr.s_addr)),ntohs(server_addr->sin_port));


	GetClientPseudo(name);
	

  //Connexion serveur
  int err = connect(sockfd, (struct sockaddr *)server_addr, sizeof(struct sockaddr_in));
  if (err == -1) {
		printf("ERREUR: connexion\n");
		return EXIT_FAILURE;
	}

	// Envoi du pseudo
	send(sockfd, name, 32, 0);

	printf("=== Bienvenue dans notre messagerie \"%s\" ===\n",name);

	pthread_t send_msg_thread;
  if(pthread_create(&send_msg_thread, NULL, (void *) envoie_msg_client, NULL) != 0){
		printf("ERREUR: pthread\n");
    return EXIT_FAILURE;
	}

	pthread_t recv_msg_thread;
  if(pthread_create(&recv_msg_thread, NULL, (void *) rec_msg_client, NULL) != 0){
		printf("ERREUR: pthread\n");
		return EXIT_FAILURE;
	}

	while (1){
		if(flag){
			printf("\nAu revoir et à bientôt !\n");
			break;
    }
	}

	close(sockfd);

	return EXIT_SUCCESS;
}


void GetClientPseudo(char *name)
{
	printf("Entrer votre pseudo : ");
	fgets(name, 32, stdin);
	for (int i = 0; i < strlen(name); i++) { // enlever \n
		if (name[i] == '\n') {
			name[i] = '\0';
			break;
		}
	}


	if (strlen(name) > 32 || strlen(name) < 2){
		printf("Le pseudo doit avoir au plus 30 caractères et au moins 2 caractères\n");
		return GetClientPseudo(name);//On redemande un pseudo jusqu'à avoir un pseudo valide
	}

}
