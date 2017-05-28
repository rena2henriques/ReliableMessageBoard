#include "utils.h"
//#include "udpServer.h"


void read_args(int argc, char const *argv[], struct sockaddr_in *id_server, struct msg_id *msg_server, int *max_msg, int *reg_int){

	int i = 1;

	if ( argc < 9 || argc % 2 != 1) {
		printf("Not sufficient arguments\n");
		exit(1);
	}


	while(i < argc) {

		if(strcmp(argv[i], "-n") == 0) {

			strcpy(msg_server->name, argv[++i]);

		}
		else if(strcmp(argv[i], "-j") == 0) {
			inet_aton(argv[++i], (void *) &(msg_server->addr.s_addr));
			//msg_server->addr.s_addr = ((struct in_addr *) argv[++i])->s_addr;
		}
		else if(strcmp(argv[i], "-u") == 0) {

			msg_server->term_port = htons( (unsigned short)	atoi(  argv[++i])	); 

		}
		else if(strcmp(argv[i], "-t") == 0) {

			msg_server->serv_port = htons( (unsigned short) atoi(argv[++i]));

		}
		else if(strcmp(argv[i], "-i") == 0) {

			inet_aton(argv[++i], (void *) &(id_server->sin_addr) );

		}
		else if(strcmp(argv[i], "-p") == 0) {
			
			id_server->sin_port = htons( (unsigned short) atoi(argv[++i]) );

		}
		else if(strcmp(argv[i], "-m") == 0) {
			
			*max_msg = atoi( argv[++i] );

		}
		else if(strcmp(argv[i], "-r") == 0) {
			
			*reg_int = atoi( argv[++i] );
		}
		else {
			printf("Not valid arguments\n");
			exit(1);
		}

		i++;

	}

	return;
}

// mete os dados default do servidor de identidades
void setDefaultSettings(struct sockaddr_in *id_server, socklen_t *addrlen, int *max_msg, int *reg_int) {

	char server_name[] = "tejo.tecnico.ulisboa.pt";
	unsigned short omission_port = 59000;

	struct hostent *h;

	if ( (h = gethostbyname(server_name) ) == NULL ) {
		printf("Error in gethostbyname: %s\n", hstrerror(h_errno) );
		exit (1);
	}

	memset( (void *) id_server, (int) '\0', sizeof(id_server));
	id_server->sin_family = AF_INET;
	id_server->sin_port = htons(omission_port);
    id_server->sin_addr.s_addr = ((struct in_addr *) (h->h_addr_list[0]))->s_addr;
    

    addrlen = (socklen_t *) sizeof(id_server);

    *max_msg = 200; // nº máx de mensagens por omissão
    *reg_int = 10; // tempo de duração do server por omissão

    return;
}

// atribui um valor à socket, testa se correu bem e atribui um novo valor ao fdmax
void setup_sockets(int * fd, int *fdmax, int domain , int type){


	if((*fd=socket(domain, type,0)) == -1) {
		printf("Error while creating a socket: %s\n", strerror(errno));
		exit(1);
	}

	*fdmax = max(*fdmax, *fd);
}


// função que faz malloc e testa-o logo de modo a diminuir nº de codigo
void * mymalloc(int size){
	void * node = (void*)malloc(size);
	if(node == NULL){
		printf("erro malloc:\n");
		exit(1);
	}
	return node;
}

// inicializa uma estrutura do tipo sockaddr_in. Serve para diminuir código
void setSockaddrs(struct sockaddr_in *addr, unsigned long ip, unsigned short port, socklen_t *addrlen) {

	memset((void*)addr, (int)'\0', sizeof(*addr));
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr=htonl(ip);
	addr->sin_port = htons(port);
	*addrlen = sizeof(*addr);

	return;
}