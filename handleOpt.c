#include "handleOpt.h"


void getServers(int fdID, struct sockaddr_in *id_server, socklen_t * addrlenID, struct serverStruct * ServStruct, char buffer[BUFFER_SIZE], int *fdmax, struct Flags *flags, struct opt2_info *opt2info, char **bufferMessages){
	
	memset(buffer, '\0', BUFFER_SIZE);

	struct sockaddr_in temp;
	socklen_t addrlenTemp = 0;
	memset(&temp, '\0', sizeof(temp));
	
	char auxbuffer[BUFFER_SIZE];

	char name[50];
	char ip[16];
	char udp_port[10];
	char tcp_port[10];

	int found = 0;
	int i = 0;	// counter de servidores a inserir no array

	// inicializa o parametro existence dos servers bloqueados a 0
	initBlocked(ServStruct);

	char delimiter[3] = "\n";
	char *token;

	if (recvfrom(fdID, buffer, BUFFER_SIZE, 0, (struct sockaddr*) id_server, addrlenID) == -1) {
		printf("error recvfrom get_servers: %s\n", strerror(errno) );
	}

	// usamos um auxbuffer para não alterar o conteudo do buffer para podermos utilizar para o show_servers
	strcpy(auxbuffer, buffer);

	memset(ServStruct->serv, '\0', MAX_SERVER);
	ServStruct->n_servers = 0;

	/****************************************************************************************************
	*			FAZER strcmp("SERVERS", token); para o caso em que o servidor de identidades pode nao responder com SERVERS
	*			NAO fazer nada ser token != SERVERS
	****************************************************************************************************/
	
	token = strtok(auxbuffer, delimiter);

	if(token == NULL) {
		return;
	}

	// lê cada servidor e armazena os dados em cada index da estrutura
	else if(!strcmp(token, "SERVERS")){
		while (token != NULL){

			token = strtok(NULL, delimiter);

			if(token == NULL)
				break;


			if(sscanf(token,"%50[^;];%16[^;];%10[^;];%10[^\n]s", name, ip, udp_port, tcp_port) == 4){
				
				inet_aton(ip, &(temp.sin_addr));
				setSockaddr(&temp, &addrlenTemp, &(temp.sin_addr), (unsigned short) atoi(udp_port));
				
				ServStruct->serv[i] = temp;
				ServStruct->n_servers++;
			



				// verifica se o servidor deste ciclo está na lista de bloqueados
				checkBlocked(temp, ServStruct, 1);

				if(compareServers(temp, ServStruct->theChosenOne) == 1){ 

					if(checkBlocked(ServStruct->theChosenOne, ServStruct, 0) == -1){

						// printf("O chosen one não está bloqueado\n");				// Comentário para debug
						found = 1;
					}
					else{
						//printf("O chosen one está bloqueado\n");			// Comentário para debug
						found = -1;
					}
				}

				i++;
			}
			else{
				printf("Unable to read server, not valid identity in identities server\n");
			}
		}
	}

	if (ServStruct->n_servers == 0) {
		printf("No servers were found.\n");
		return;
	}

	// Apaga todos os servidores bloqueados que já não estiverem registados
	resetBlocked(ServStruct);

	//printf("n de bloqueados: %d nº de servidores: %d\n", ServStruct->numBlocked, i); // Comentário para debug

	// se todos os servidores estiverem bloqueados
	if(ServStruct->numBlocked == i) {

		printf("The messages can't be sent to any server. Try later.\n");
		PROMPT;

		// limpa todas as flags assinaladas
		flags->publish = 0;
		flags->showLatMsg = 0;
		flags->msgTest = 0;

		if(*bufferMessages != NULL){
			free(*bufferMessages);
			*bufferMessages = NULL;
		}
		// remove todas as operações pendentes
		freeOptList(opt2info, 1);
	}
	// se o servidor de mensagens que usavamos para fazer as ligações já lá não estava escolhemos outro
	else if(found != 1)	{

		// se houver mais do que um servidor
		if(ServStruct->n_servers > 1 )	{
			//printf("Vou gerar um novo chosen one\n"); 			// Comentário para debug
			genServer(ServStruct);
		}
		// se houver um só servidor
		else if(ServStruct->n_servers == 1) {
			ServStruct->theChosenOne = ServStruct->serv[0];
		}
	}

	return;
}


// Resolve as operações desejadas
void handleOpt(struct Flags *flags, struct serverStruct *ServStruct, struct opt2_info *opt2info, char buffer[BUFFER_SIZE], char publishBuffer[180], char showLatMsgBuffer[50], int fdID, socklen_t *addrlenID, struct sockaddr_in *id_server, char **bufferMessages) {

	// no caso de ser a resposta a uma operação Show_servers
	if( flags->showServers > 0 && opt2info->head->optype == SHOW_SERVERS){
		printf("\x1b[48;5;88m%s\x1b[0m\n", buffer);
		flags->showServers--;
		removeOpt(opt2info);
		memset(buffer, '\0', BUFFER_SIZE);
		PROMPT;
	}
	// no caso de ser a resposta a uma operação publish msg
	else if( flags->publish > 0 && opt2info->head->optype == PUBLISH){
		
		memset(publishBuffer, '\0', 180);

		strcpy(publishBuffer, "PUBLISH ");
		// junto a mensagem ao PUBLISH
		strcat(publishBuffer, opt2info->head->opt);

		WAITING;
		//printf("enviei a mensagem: %s : para %s com port %hu\n", publishBuffer, inet_ntoa(ServStruct->theChosenOne.sin_addr), ntohs(ServStruct->theChosenOne.sin_port));
		// envio a mensage para o servidor de mensagens escolhido
		if( sendToServer(ServStruct->fd, &(ServStruct->theChosenOne), &(ServStruct->addrlen), publishBuffer) == 1){

			memset(showLatMsgBuffer, '\0', 50);

			// chama-se o show_latest_messages para verificarmos se a mensagem foi enviada com sucesso
			// 5 é um nº aleatorio, pode ter que ser alterado 
			strcpy(showLatMsgBuffer, "GET_MESSAGES 5");

			//printf("Vou alocar memória para o bufferMessages na linha 159: %s\n", *bufferMessages);
			if(*bufferMessages != NULL){
				free(*bufferMessages);
				*bufferMessages = NULL;
			}
			bufferMessagesSize = (12 + 140*5);

			*bufferMessages = (char *) mymalloc( bufferMessagesSize *sizeof(char));

			//printf("Aloquei memória para o bufferMessages na linha 159: %s\n", *bufferMessages);

			sendToServer(ServStruct->fd, &(ServStruct->theChosenOne), &(ServStruct->addrlen), showLatMsgBuffer);

			// incrementamos a flag para indicar que vamos aguardar uma confirmação de que recebemos a mensagem
			flags->msgTest++;

		}
		else {
			flags->msgTest++;
		}

	}
	// no caso de ser a resposta a uma operação show_latest_messages n
	else if (flags->showLatMsg > 0 && opt2info->head->optype == SHOW_LATEST_MESSAGES) {

		memset(showLatMsgBuffer, '\0', 50);

		strcpy(showLatMsgBuffer, "GET_MESSAGES ");

		strcat(showLatMsgBuffer, opt2info->head->opt);

		//printf("Vou alocar memória para o bufferMessages na linha 187: %s\n", *bufferMessages);

		if(*bufferMessages != NULL){
			free(*bufferMessages);
			*bufferMessages = NULL;
		}
		bufferMessagesSize = (12 + 140*( (int) atoi(opt2info->head->opt) ) );

		*bufferMessages = (char *) mymalloc( bufferMessagesSize* sizeof(char) );

		//printf("Aloquei memória para o bufferMessages na linha 187: %s\n", *bufferMessages);
		WAITING;
		//printf("enviei o pedido de %s mensagens: para %s com port %hu\n", opt2info->head->opt, inet_ntoa(ServStruct->theChosenOne.sin_addr), ntohs(ServStruct->theChosenOne.sin_port));
		if( sendToServer(ServStruct->fd, &(ServStruct->theChosenOne), &(ServStruct->addrlen), showLatMsgBuffer) == 1) {


			// limpa-se já o buffer porque não irá ser utilizado em diante
			memset(showLatMsgBuffer, '\0', 50);
			flags->msgTest++;

		}
		else {
			flags->msgTest++;
		}
	}


}

// para o caso de termos contactado mais do que 3 vezes o servidor de mensagens e não tivermos obtido resposta
int counterExceeded( struct Flags *flags, struct serverStruct * ServStruct, int fdID, socklen_t *addrlenID, struct sockaddr_in *id_server, struct opt2_info * opt2info){

	if(flags->msgTest > 3) {

		printf("Selecting new server to connect.\n");

		// inserir nos bloqueados
		insertBlocked(ServStruct, ServStruct->theChosenOne);
		ServStruct->numBlocked++;

		// vai voltar a ter que testar as mensagens
		flags->msgTest = 0;

		if( sendToServer(fdID, id_server, addrlenID, "GET_SERVERS") == -1){
			printf("Can't connect to identity Server.\n");

			// dar free de tudo
			close(ServStruct->fd);
			close(fdID);
			freeBlocked(&(ServStruct->head));
			freeOptList(opt2info, 0);

			exit(1);
		}

		return 1;
	}

	return -1;
}