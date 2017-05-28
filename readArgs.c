#include "readArgs.h"


void read_args(int argc, char const *argv[], struct sockaddr_in *id_server){

	/* no caso de ser só rmb */
	switch(argc){

		case 1:
			/* se só houver um argumento, rmb, então por omissão o ip do id_server é o do tejo e a porta é a 59000 */
			break;

		case 2:
			printf("Chamada inválida, o número de argumentos é inválido\n");
			exit(1);
			break;

		case 3:
			if ( strcmp(argv[1], "-i") == 0) {

				if(inet_aton(argv[2], (void *) &(id_server->sin_addr)) == 0){
					printf("Not a valid IP address. Check it and rerun please\n");					
					exit(1);
				}

			}
			else if ( strcmp(argv[1], "-p") == 0) {
				id_server->sin_port = htons( (unsigned short) atoi(argv[2]) );
			}
			else {
				printf("Chamada inválida, os argumentos são inválido\n");
				exit (1);
			}
			break;

		case 4:
			printf("Chamada inválida, o número de argumentos é inválido\n");
			exit(1);
			break;
	
		case 5:
			if ( strcmp(argv[1], "-i") == 0) {
				inet_aton(argv[2], (void *) &(id_server->sin_addr) );
			}
			else if ( strcmp(argv[1], "-p") == 0) {
				id_server->sin_port = htons( (unsigned short) atoi(argv[2]) );
			}
			else {
				printf("Chamada inválida, os argumentos são inválidos\n");
				exit (1);
			}
		
			if ( strcmp(argv[3], "-i") == 0) {
				inet_aton(argv[4], (void *) &(id_server->sin_addr) );
			}
			else if ( strcmp(argv[3], "-p") == 0) {
				id_server->sin_port = htons( (unsigned short) atoi(argv[4]) );
			}
			else {
				printf("Chamada inválida, os argumentos são inválidos\n");
				exit (1);
			}
			break;


		default:
			printf("Chamada inválida, o número de argumentos é inválido (default)\n");
			exit (1);
			break;
	}

 	return;
}

// dá os parametros de omissão ao servidor de identidades
// Recebe a estrutura que guarda a identidade do Servidor de Indentidades
void setDefaultSettings(struct sockaddr_in *id_server, socklen_t *addrlen) {

	char server_name[] = "tejo.tecnico.ulisboa.pt";
	unsigned short omission_port = 59000;

	struct hostent *h;

	if ( (h = gethostbyname(server_name) ) == NULL ) {
		printf("Error in gethostbyname: %s\n", hstrerror(h_errno) );
		exit (1);
	}

	// É preparada a socket em que se dará a interação com o Servidor de Identidades
	setSockaddr(id_server, addrlen, (struct in_addr *) (h->h_addr_list[0]), omission_port);

    return;
}

