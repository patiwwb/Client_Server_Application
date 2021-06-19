#include "serveur.h"


int main(int argc, char **argv){
	int init = 0;
	
	fdJournal = open(NOM_JOURNAL, O_CREAT|O_WRONLY|O_APPEND, 0600);
  	if (fdJournal == -1)
    erreur_IO("ouverture journal");
	
	if(argc != 2){
		printf("Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}

	int port = atoi(argv[1]);
	int option = 1;
	int listenfd = 0, connfd = 0;
  struct sockaddr_in serv_addr;
  struct sockaddr_in cli_addr;
  pthread_t tid;


  /* Param socket*/
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr =INADDR_ANY;
  serv_addr.sin_port = htons(port);

  /* Ignorer SIGPIPE*/
	signal(SIGPIPE, SIG_IGN);

	if(setsockopt(listenfd, SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0){
		perror("ERREUR: setsockopt échec");
    return EXIT_FAILURE;
	}

	/* Bind */
  if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("ERREUR: Socket bind échec");
    return EXIT_FAILURE;
  }

  /* Listen */
  if (listen(listenfd, 0) < 0) {
    perror("ERREUR: Socket listen échec");
    return EXIT_FAILURE;
	}
	
	time_t mytime = time(NULL);
    char * time_str = ctime(&mytime);
    time_str[strlen(time_str)-1] = '\0';
    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    sprintf(time_str, "[%d %d %d %d:%d:%d]",timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    
    printf("\nDémarrage du serveur %s\n", time_str);
	printf("\n========== SERVEUR =============\n");
	


	while(1){
		if(!init)
		{
			printf(" En attente de connexion\n");
		}
		init = 1;
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);
		
		if (connfd < 0)
			erreur_IO("accepte");


		 printf("%s:%hu se connecte\n", stringIP(ntohl(cli_addr.sin_addr.s_addr)), ntohs(cli_addr.sin_port));

		/* Vérifier place client dispo */
		if((cli_count + 1) == MAX_CLIENTS){
			printf("Nbr de clients max atteint. Refus de: (%s:%hu)\n ",stringIP(ntohl(cli_addr.sin_addr.s_addr)), ntohs(cli_addr.sin_port));
			close(connfd);
			continue;
		}

		/* Param client */
		Client *cli = (Client *)malloc(sizeof(Client));
		cli->address = cli_addr;
		cli->sockfd = connfd;
		cli->uid = uid++;

		/* Ajouter client et gérer thread */
		queue_push(cli);
		pthread_create(&tid, NULL, &com_client, (void*)cli);

		/* Mettre en pause*/
		sleep(1);
	}

	return EXIT_SUCCESS;
}

void remiseAZeroJournal(void) {
  // on ferme le fichier et on le rouvre en mode O_TRUNC

  if (close(fdJournal) == -1)
    erreur_IO ("fermeture journal pour remise a zero");

  	fdJournal = open(NOM_JOURNAL, O_TRUNC|O_WRONLY|O_APPEND, 0600);

  if (fdJournal == -1)
    erreur_IO ("reouverture journal pour remise a zero journal");
}
