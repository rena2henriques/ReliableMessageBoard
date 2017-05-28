#include "udpServer.h"

// lê o que recebeu de um terminal e decide que operação fazer
void bufferAnalyser(char buffer[BUFFER_SIZE], struct msgs * msgStruct, int fd, struct sockaddr *addr, struct servers_info *myServer, fd_set *rfds){


	//	ler:
	//		PUBLISH message
	//		GET_MESSAGES n
	//	responde:
	//		MESSAGES\n(message\n)
	//

	char op[msgSize] = "";
	char token[20] = "";
	char * answer = NULL;

	// inicialização
	op[0]='\0';

	// lê o tipo de mensagem que recebeu
	if(sscanf(buffer, "%20s %140[^\n]s", token, op) != 2){

		answer = (char *)mymalloc(sizeof("NOK - unknown command\n"));
		strcpy(answer, "NOK - unknown command\n");
	}
	// vê qual foi o tipo de pedido requisitado
	else{
		if(!strcmp(token,"PUBLISH") && strlen(op)){

			msgStruct->curMsgNumber += insertMsgList(op, &(msgStruct->head), msgStruct, msgStruct->LC);
			msgStruct->LC++;
			// enviar para todos os servidores conectados 
			shareMsg(msgStruct, myServer, rfds);

		}

		else if(!strcmp(token,"GET_MESSAGES")){
			if( atoi(op) != 0){
				getMessages(&answer, msgStruct, atoi(op));		
			}
			else{
				answer = (char *)mymalloc(sizeof("MESSAGES\n"));
				strcpy(answer, "MESSAGES\n");
			}
		}
		else{
			answer = (char *)mymalloc(sizeof("NOK - unknown command\n"));
			strcpy(answer, "NOK - unknown command\n");
		}
	}
	
	// envia o pedido
	if(answer != NULL){
		reply(answer, fd, addr);
		free(answer);
	}
}

// Insere a mensagem recebida por UDP na lista de mensagens
int insertMsgList(char data[msgSize], struct msgList ** head, struct msgs * msgStruct, int LC){ 

	struct msgList * temp = NULL;
	
	temp = (struct msgList *) mymalloc(sizeof(struct msgList));

	struct msgList * aux = (*head);
	struct msgList * prev = aux;

	strcpy(temp->msg, data);

	// atribui-lhe um nº de clock lógico
	temp->msgTime = LC; 

	// se o nº de menagens já tiver excedido o limite então remove-se a ultima para dar entrada à nova
	if( msgStruct->curMsgNumber >= msgStruct->maxMsgNumber){
		remove_list_tail(head);
	}

	// insere ordenadamente por LCs
	if(*head == NULL)	{
		(*head) = temp;	
		(*head)->next = NULL;
		return 1;
	}
	if((*head)->next == NULL){
		if((*head)->msgTime < LC){
			temp->next = (*head);
			(*head) = temp;
			return 1;
		}
		else{
			temp->next = NULL;
			(*head)->next = temp;
			return 1;
		}
	}

	while(aux != NULL){

		if(aux->msgTime < LC){
			temp->next = aux;

			if(aux == (*head)){
				(*head) = temp;
			}
			else{
				prev->next = temp;
			}
			return 1;
		}
		prev = aux;
		aux = aux->next;
	}

	temp->next = aux;
	prev->next = temp;

	return 1;

}

// Responde ao show_latest_messages
void getMessages(char ** answer, struct msgs * msgStruct, int n){
	
	int sizeofBuffer = 0;
	int counter = 0; // utilizado no fill with servers para ver quantas mensagens queremos enviar

	// Definir o tamanho do buffer para que ele se ajuste ao nº de mensagens que temos guardadas
	if( msgStruct->curMsgNumber > msgStruct->maxMsgNumber) {
		sizeofBuffer = 11 + 143*msgStruct->maxMsgNumber;
	}
	else
		sizeofBuffer = 11 + 143*msgStruct->curMsgNumber;

	char *buffer = (char *) mymalloc( (sizeofBuffer + 1) * sizeof(char));

	strcpy(buffer, "MESSAGES\n");
	
	// envia um certo nº de mensagens que foram pedidas
	fillWithMessages(buffer, msgStruct->head, n, &counter);

	*answer = (char*) mymalloc( sizeof(char)*( strlen(buffer) + 1));

	strcpy(*answer,buffer);

	free(buffer);

	return;

}

// regista-se no servidor de identidades
void join(struct sockaddr_in *id_server, struct msg_id *msg_server, int fd) {

	int n = 0; 				// retorno do sendto
	char msg[110] = "REG ";
	char info[100] = "";
	memset(info, '\0', 100);

	int flagJoinTest = 0;	// flag para irmos testar se o join no servidor de id foi bem feito

	sprintf(info,"%s;%s;%hu;%hu",msg_server->name, inet_ntoa(msg_server->addr),ntohs(msg_server->term_port), ntohs(msg_server->serv_port));
	strcat(msg, info);

	// while o teste não foi bem sucedido, ou seja, não estamos registados no servidor de identidades
	while (flagJoinTest == 0) {

		n = sendto(fd, msg, strlen(msg), 0, (struct sockaddr*) id_server, sizeof(*id_server));

		if ( n == -1 ) {
			printf("fd:%d\n",fd );
			printf("error sendto: %s\n", strerror(errno) );
			printf("Can't connect to identity Server. Aborting.\n");
			exit(1);
		}

		joinTest(id_server, msg_server, fd, info, &flagJoinTest);

	}

	return;
}

// Testa se o join foi de facto enviado
void joinTest(struct sockaddr_in *id_server, struct msg_id *msg_server, int fd, char id[100], int *flagJoinTest) {

	int counter = 0;
	fd_set rfds;
	struct timeval tv;
	int tries = 0;

	tv.tv_sec = 3;									// se o recvfrom não tiver sucesso em 2 segundos, o programa desiste
	tv.tv_usec = 0;

	int n = 0; 			// retorno do sendto
	char buffer[BUFFER_SIZE] = "";
	memset(buffer, '\0', BUFFER_SIZE);
	socklen_t addrlen = sizeof(id_server);

	while(tries < 3) {

		n = sendto(fd, "GET_SERVERS", 11, 0, (struct sockaddr*) id_server, sizeof(*id_server));
		if ( n == -1 ) {
			printf("error sendto get_servers: %s\n", strerror(errno) );
			printf("Can't connect to identity server. Aborting.\n");
			exit(1);
		}

		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);

		// para definir o timer
		counter = select(fd + 1, &rfds, (fd_set *) NULL, (fd_set *) NULL, &tv);
		if( counter < 0) {
			printf("Error in select of get_servers - %s\n", strerror(errno));
			exit(1);
		}

		if(counter == 0){
			printf("Waiting time to connect to identity server expired. Wait a little longer.\n");
			tries++;
		}

		if( FD_ISSET(fd, &rfds) ) {
			n = recvfrom(fd, buffer, BUFFER_SIZE, 0 , (struct sockaddr *) id_server, &addrlen);
			if( n == -1){
				printf("error in recvfrom of get_message of id_server %s\n", strerror(errno));
				exit(1);
			}

			break;
		}

	}

	if( tries >= 3) {
		printf("The connection to the identity server can't be done. Aborting Program.\n");
		exit(1);
	}

	// depois de pedir os servidores ao id server vamos ver se o nosso registo está de facto lá
	if ( strstr(buffer, id) == NULL ) {
		*flagJoinTest = 0;
	}
	else {
		*flagJoinTest = 1;
	}
	
	//printf("O teste do Join deu: %d \n", *flagJoinTest);   // Debug

	return;
}



// remove o ultimo elemento da lista 
void remove_list_tail(struct msgList ** head) {

	struct msgList * temp;
	struct msgList * aux;

	temp = (*head);
	aux = temp;

	while(temp->next != NULL) {

		aux = temp;
		temp = temp->next;
	}

	aux->next = NULL;
	free(temp);

	return;
}

// envia o pretendido para o rmb
void reply(char * answer, int fd, struct sockaddr * addr){
	socklen_t addrlen = (socklen_t)sizeof(*addr);
	int n = sendto(fd, answer, strlen(answer), 0, addr, addrlen);
	if ( n == -1 ) {
		printf("error reply: %s\n", strerror(errno) );
		exit(1);
	}
}


// liberta a mem de toda a lista de mensagens
void freeMsgList(struct msgs *msgStruct){

	struct msgList *auxiliar = NULL;

	while(msgStruct->head != NULL) {

		auxiliar = msgStruct->head;
		msgStruct->head = msgStruct->head->next;
		free(auxiliar);
	}

	return;
}

// recursivamente junta as mensagens de modo a que a mais antiga fique em cima e a mais recente em baixo
void fillWithMessages(char *buffer, struct msgList *node, int n, int *counter) {

	if(node == NULL || n == *counter){
		return;
	}

	(*counter)++;
	fillWithMessages(buffer, node->next, n, counter);

	strcat(buffer, node->msg);
	strcat(buffer, "\n");

	return;
}