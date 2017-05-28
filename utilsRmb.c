#include "utilsRmb.h"


int sendToServer(int fd, struct sockaddr_in *gate, socklen_t * addrlen, char  * message){

	if ( sendto(fd, message, (int)strlen(message), 0, (struct sockaddr*) gate, sizeof(*gate)) == -1 ) {
		printf("error sending: %s\nerron: %s\n", message, strerror(errno) );
		return -1;
	}
	return 1;
}


void setSockaddr( struct sockaddr_in * server, socklen_t *addrlen, struct in_addr * addr, unsigned short port){

	struct in_addr temp = (*addr);
	temp.s_addr = addr->s_addr;

	memset((void *)server, '\0', sizeof(server));
	server->sin_addr.s_addr = temp.s_addr;
	server->sin_port = htons(port);
	server->sin_family = AF_INET;
	addrlen = (socklen_t *)sizeof(*server);

	return;
}

void sigPipe(){
	void (*old_handler)(int);
		if( (old_handler = signal(SIGPIPE, SIG_IGN)) == SIG_ERR) {
			printf("Houve um conflito a ignorar os sinais SIGPIPEs\n");
			exit(1);
		}
}

void setup_sockets(int *fd, int *fdmax) {

	*fd = socket(AF_INET, SOCK_DGRAM, 0); // SOCK_DGRAM = UDP
	if (*fd == -1) {
		printf("error socket: %s\n", strerror(errno) );
		exit(1);
	}

	*fdmax = max(*fdmax, *fd);

	return;
} 

void * mymalloc(int size){
	void * node = (void*)malloc(size);
	if(node == NULL){
		printf("erro malloc:\n");
		exit(1);
	}
	return node;
}

// função que imprime a lista de operações <- só para debug
void printOpt(struct opt2_list * node){
	if(node == NULL){
		return;
	}
	printOpt(node->next);
	printf("type:%d msg:%s\n", node->optype , node->opt);

	return;
}


//insere uma operacao no fim da lista
void insertOpt(char buffer[141], int type, struct opt2_info * opt2info){

	struct opt2_list * temp = (struct opt2_list *) mymalloc( sizeof(struct opt2_list) );
	struct opt2_list * aux = opt2info->head;

	strcpy(temp->opt, buffer);
	temp->optype = type;

	if(opt2info->head ==NULL){
		temp->next =NULL;
		opt2info->head = temp;

		opt2info->listSize++;	
		return;
	}

	//percorre até ao fim
	while( aux->next != NULL){
		aux = aux->next;
	}

	aux->next = temp;
	temp->next = NULL;
	
	opt2info->listSize++;	
	return;
}

//remove operacao tratada, do inicio da lista
void removeOpt(struct opt2_info * opt2info){

	struct opt2_list * aux = opt2info->head;

	if(opt2info->head == NULL){
		return;
	}

	opt2info->head = aux->next;
	aux->next = NULL;
	
	free(aux);

	opt2info->listSize--;

	return;
}

// dá free da lista toda
void freeOptList(struct opt2_info * opt2info, int flag) {

	struct opt2_list * aux = opt2info->head;
	struct opt2_list * prev = aux;

	while(flag == 0 && opt2info->head != NULL) {
		removeOpt(opt2info);
	}

	while(flag == 1 && aux != NULL){
		
		if(aux->optype != SHOW_SERVERS){
			if( aux == opt2info->head){
				opt2info->head = opt2info->head->next;
				free(aux);
				opt2info->listSize--;
				aux = opt2info->head;
				prev = opt2info->head;
				continue;
			}
			else{
				prev->next = aux->next;
				free(aux);
				aux = prev;
			}
		}
		
		prev = aux;
		aux = aux->next;
	}

	return;
}

// compara dois servidores
int compareServers(struct sockaddr_in new, struct sockaddr_in server){

	if(new.sin_addr.s_addr == server.sin_addr.s_addr && new.sin_port == server.sin_port){
		return 1;
	}
	else
		return -1;
}

// testa se a mensagem enviada para o servidor de msgs foi recebida ao procurá-la no buffer do show_latest_messages
int checkMsgPlace(char *buffer, struct opt2_info * opt2info) {

	if (strstr(buffer, opt2info->head->opt) != NULL){
		return 1;										// se foi encontrada a mensagem no buffer do show_latest_mensages
	}
	else
		return -1;										// se não foi encontrada a mensagem no buffer do show_latest_mensages


}

// inicializa o parametro existence da lista de banidos a 0
void initBlocked(struct serverStruct *servStruct){

	struct Blocked * aux = servStruct->head;

	while(aux != NULL){

		aux->existence = 0;
		aux = aux->next;
	}

	return;
}

/// Retorna 1 se NEW estiver Bloqueado
int checkBlocked(struct sockaddr_in new, struct serverStruct  * ServStruct, int flag){
	struct Blocked * aux = ServStruct->head;
	while(aux != NULL){
		if(compareServers(new, aux->id) == 1 ){
			aux->existence += flag; // se a flag for 1 ele incrementa, se for 0 não
			return 1;
		}
		aux = aux->next;
	}
	return -1;
}

// remove um servidor que deixou de estar registado no servidor de identidades
void resetBlocked(struct serverStruct * ServStruct){
	struct Blocked * aux = ServStruct->head;
	struct Blocked * prev = aux;

	while(aux != NULL){
		
		if(aux->existence == 0){

			if( aux == ServStruct->head){

				ServStruct->head = ServStruct->head->next;
				free(aux);
				ServStruct->numBlocked--;
				aux = ServStruct->head;
				prev = ServStruct->head;
				continue;
			}
			else{
				prev->next = aux->next;
				ServStruct->numBlocked--;
				free(aux);
				aux = prev;
			}
		}
		
		prev = aux;
		aux = aux->next;
	}

}


// Insere na lista dos servidores bloqueados
void insertBlocked (struct serverStruct *ServStruct, struct sockaddr_in blocked){

	struct Blocked * new = (struct Blocked *) mymalloc(sizeof(struct Blocked));
	new->counter = 0;
	new->existence = 0;
	new->id = blocked;

	if(ServStruct->head == NULL){
		ServStruct->head = new;
		new->next = NULL;
	}
	else{
		new->next = ServStruct->head;
		ServStruct->head = new;
	}

	return;
}



// liberta a lista dos servidores bloqueados
void freeBlocked (struct Blocked **aux) {
	if((*aux) == NULL)
		return;

	freeBlocked(&(*aux)->next);
	free((*aux));
	(*aux) = NULL;
	return;
}


// escolher um servidor para ser o chosen one que não esteja bloqueado
void genServer(struct serverStruct * ServStruct) {

	int n_random = rand() % ServStruct->n_servers;

	//printf("nº de bloqueados: %d nº de servidores: %d\n", ServStruct->numBlocked, ServStruct->n_servers); // Comentário para debug

	while( checkBlocked(ServStruct->serv[n_random], ServStruct, 0) == 1) {

		// printf(" o server com ip = %s e port = %hu\n", inet_ntoa(ServStruct->serv[n_random].sin_addr), ntohs(ServStruct->serv[n_random].sin_port));	// debug

		n_random = rand() % ServStruct->n_servers;
	}

	//printf("novo nº random = %d\n", n_random);		// Debug

	ServStruct->theChosenOne = ServStruct->serv[n_random];

	return;
}
