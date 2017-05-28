#include "tcpServer.h"

// pergunta ao servidor de identidades os servidores de mensagens que lá tem registados
void get_servers(struct sockaddr_in *id_server, struct servers_info *myServer, struct msg_id *serverInfo) {


	int fd;
	int fdmax = 0, counter = 0;
	fd_set rfds;
	struct timeval tv;
	int tries = 0;

	tv.tv_sec = 5;																// se o recvfrom não tiver sucesso em 3 segundos, o programa desiste
	tv.tv_usec = 0;

	int n;
	char buffer[BUFFER_SIZE];
	memset(buffer, '\0', BUFFER_SIZE);
	socklen_t addrlen = sizeof(id_server);

	setup_sockets(&fd, &fdmax, AF_INET, SOCK_DGRAM);					

	while(tries < 3) {

		n = sendto(fd, "GET_SERVERS", 11, 0, (struct sockaddr*) id_server, sizeof(*id_server));
		if ( n == -1 ) {
			printf("error sendto get_servers: %s\n", strerror(errno) );
			printf("Can't connect to identity server. Aborting.\n");
			exit(1);
		}

		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);

		counter = select(fdmax + 1, &rfds, (fd_set *) NULL, (fd_set *) NULL, &tv);
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
				printf("error in recvfrom of get_message %s\n", strerror(errno));
				exit(1);
			}

			break;
		}

	}

	if( tries >= 3) {
		printf("The connection to the identity server can't be done. Aborting Program.\n");
		exit(1);
	}

	// meter a info sacada do GET_MESSAGES na lista de servidores
	read_getservers(buffer, myServer, serverInfo);

	close(fd);

	return;
}

// transforma o retorno do servidor de identidades em várias variaveis
void read_getservers(char buffer[BUFFER_SIZE], struct servers_info *myServer , struct msg_id *serverInfo) {

	char name[50] = "";
	char ip[16] = "";
	char udp_port[10] = "";
	char tcp_port[10] = "";


	char delimiter[3] = "\n";
	char *token = NULL;

	// mostra o que está no servidor de identidades
	//printf("\x1b[48;5;195m%s\x1b[0m", buffer);	// Debug

	token = strtok(buffer, delimiter);

	// Se a mensagem não tiver nada
	if(token == NULL) {
		return;
	}

	// lê cada servidor e armazena os dados em cada index da estrutura
	while (token != NULL){

		token = strtok(NULL, delimiter);

		if(token == NULL)
			break;

		if( sscanf(token,"%50[^;];%16[^;];%10[^;];%10[^\n]s", name, ip, udp_port, tcp_port) == 4 ) {

			// Para impossibilitar que o nosso server se conecte a ele próprio
			if( strcmp( inet_ntoa(serverInfo->addr), ip) == 0 )  {
			  if ( ntohs(serverInfo->serv_port) == (unsigned short) atoi(tcp_port) ) {
			  	continue;
			  }
			}

			// preencher nodes da lista
			fill_servers_list( myServer, ip , tcp_port, -1); 
		}
		else {
			printf("One of the servers in the identity server didn't have a valid identity.\n");
		}
	}


	return;
}	

// insere os dados de cada servidor de mensagens na lista
void fill_servers_list(struct servers_info *myServer, char ip[16], char tcp_port[10], int fd) {

	struct servers_list * temp = NULL;
	
	temp = (struct servers_list *) mymalloc( sizeof(struct servers_list ) );

	// insere sempre pela head
	if(myServer->head == NULL)	{
		myServer->head = temp;	
		myServer->head->next = NULL;
	}
	else{
		temp->next = myServer->head;
		myServer->head = temp;
	}

	// converte de string para ip
	if( inet_aton(ip, &( myServer->head->addr.sin_addr ) )  == 0 ) {
		printf("the conversion from ip to sin_addr was invalid\n");
	}

	// inicializa os dados do servidor
	myServer->head->addr.sin_port = htons( (unsigned short) atoi(tcp_port) );
	myServer->head->addr.sin_family = AF_INET;
	myServer->head->fd_tcp = fd;
	myServer->head->flagMsg = 0;
	// inicializa a str auxiliar a 0
	memset(myServer->head->auxstr, '\0', 160);

	// incrementa o nº de servidores guardados na lista
	myServer->n_servers++;
	
	return;
}

// remover da lista um servidor que se desconectou 
void removeServer(struct servers_list **head, int fd) {

	struct servers_list *temp = *head;
	struct servers_list *aux = NULL;

	//printf("Servidor %s - %hu vai ser removido.\n", inet_ntoa((*head)->addr.sin_addr), ntohs((*head)->addr.sin_port)); // Debug

	while( temp != NULL) {

		if(temp->fd_tcp == fd){

			if(aux == NULL){
				*head = temp->next;
			}
			else {
				aux->next = temp->next;
			}

			close(temp->fd_tcp);
			free(temp);

			return;
		}

		aux = temp;
		temp = temp->next;		

	}

	return;
}


// percorre a lista dos servidores registados ao servidor de identidades e conecta-se a eles um a um
void make_connections(struct servers_info *myServer, fd_set *rfds, int *fdmax) {

	struct servers_list * temp = myServer->head;
	struct servers_list * auxiliar = NULL;

	while(temp != NULL) {

			// conecta-se a cada um dos servidores. se a conexão for mal sucedida é fechada a socket e o servidor é removido da lista
			if( connect_2_server(&temp, rfds, fdmax) == -1) {
				FD_CLR(temp->fd_tcp, rfds);
				auxiliar = temp->next;
				removeServer(&(myServer->head), temp->fd_tcp);
				myServer->n_servers--;
				temp = auxiliar;
			}
			else {
				FD_SET(temp->fd_tcp, rfds);
				temp = temp->next;
			}
	}

	printf("\n");


	return;
}

// conecta-se a um servidor lido do servidor de identidades 
int connect_2_server(struct servers_list **node, fd_set *rfds, int *fdmax ) {

	int conn_return;	// retorno da função connect
	struct timeval tv;	// timer para o connect
	fd_set temp_set;	// rfds para este select
	socklen_t lon = sizeof(int);	// para colocar o fd em non_blocking
	int valopt = 0;		// é preenchido quando se vê o getsockopt
	long arg;			// retorno da função para voltar a meter blocking

	// setup da socket
	setup_sockets(&( (*node)->fd_tcp ), fdmax, AF_INET , SOCK_STREAM);

	// coloca a socket como NON_BLOCKING
	if( fcntl((*node)->fd_tcp, F_SETFL, O_NONBLOCK) < 0) {
		printf("Error fcntl: %s\n", strerror(errno));
		return -1;
	}

	// printf("conexão: - fd: %d ip: %s port: %hu\n", (*node)->fd_tcp, inet_ntoa((*node)->addr.sin_addr), ntohs((*node)->addr.sin_port) );   // Debug

	// faz connect
	conn_return = connect( (*node)->fd_tcp , (struct sockaddr *) &( (*node)->addr ), (socklen_t) sizeof((*node)->addr) );

	// se ainda não tiver sido feita a conexão
	if(conn_return < 0 ){

		// se a conexão ainda estiver a tentar ser feita
		if (errno == EINPROGRESS) { 

			// timer de para o connect
	        tv.tv_sec = 2; 
	        tv.tv_usec = 0; 
	        FD_ZERO(&temp_set); 
	        FD_SET((*node)->fd_tcp, &temp_set); 

	        // Usa-se o select para definir o timer. Só se sai do select se o connect for feito ou se o timer exceder
	        if ( select((*node)->fd_tcp +1, NULL, &temp_set, NULL, &tv) > 0) {

	        	// Vê o estado da socket: se der algum erro significa que nao foi bem feito, caso contrário foi
	           getsockopt((*node)->fd_tcp, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon); 
	           if (valopt) { 
	              //fprintf(stderr, "Error in connection() with server( fd: %d ip: %s port: %hu) -> %d - %s\n", (*node)->fd_tcp, inet_ntoa((*node)->addr.sin_addr), ntohs((*node)->addr.sin_port), valopt, strerror(valopt)); 
	              PROMPT_TIMEOUT;
	              return -1; 
	           } 
	           //printf("The connection was successfull with the server: - fd: %d ip: %s port: %hu\n", (*node)->fd_tcp, inet_ntoa((*node)->addr.sin_addr), ntohs((*node)->addr.sin_port) );
	        	PROMPT_CONNECT;
	        } 
	        // significa que o select retornou 0 ou menos, ou seja, deu erro ou o tempo foi excedido
	        else { 
	           //fprintf(stderr, "Timeout or error() with server( fd: %d ip: %s port: %hu) -> %d - %s\n", (*node)->fd_tcp, inet_ntoa((*node)->addr.sin_addr), ntohs((*node)->addr.sin_port), valopt, strerror(valopt)); 
	           PROMPT_TIMEOUT;
	           return -1; 
	        } 
	    } 
	    // a conexão foi logo mal sucedida
	    else { 
	        //fprintf(stderr, "Error connecting with server( fd: %d ip: %s port: %hu) -> %d - %s\n", (*node)->fd_tcp, inet_ntoa((*node)->addr.sin_addr), ntohs((*node)->addr.sin_port), errno, strerror(errno));
	        PROMPT_TIMEOUT;
	        return -1; 
    	} 
	}

	// Voltar a colocar o file descriptor em modo de bloqueio 
	arg = fcntl((*node)->fd_tcp, F_GETFL, NULL); 
	arg &= (~O_NONBLOCK); 
	if( fcntl((*node)->fd_tcp, F_SETFL, arg) < 0){
		printf("Error fcntl: %s\n", strerror(errno));
		return -1;
	}	

	return 0;
}

// serve para dar set de todos os fd's da lista de servidores de mensagens
void fdSET(struct servers_list *node, fd_set *rfds) {

	while (node != NULL) {

		// para o caso de o fd ter sido inicializado mas ainda não tiver um valor atribuido
		if( node->fd_tcp >= 0) {
			FD_SET(node->fd_tcp, rfds);
		}

		node = node->next;
	}

	return;
}

// preenche o buffer com as mensagens
void addMessages(char str[3], char *buffer, struct msgList *node, int flag){

	if (node == NULL || flag == 0){
		memset(buffer, '\0', strlen(buffer));
		strcpy(buffer, "SMESSAGES\n");
		return;
	}

	// recursiva
	addMessages(str, buffer, node->next, --flag);

	sprintf(str, "%d", node->msgTime);
	strcat(buffer, str);
	strcat(buffer, ";");
	strcat(buffer, node->msg);
	strcat(buffer, "\n");

	return;
}

// envia mensagens para um determinado servidor por TCP
int sendMessages(struct msgs *msgStruct, int fd, struct servers_info *myServer, fd_set *rfds, int flag) {

	int sizeofBuffer = 0;

	// Definir o tamanho do buffer para que ele se ajuste ao nº de mensagens que temos guardadas
	if( msgStruct->curMsgNumber > msgStruct->maxMsgNumber) {
		sizeofBuffer = 11 + 143*msgStruct->maxMsgNumber;
	}
	else
		sizeofBuffer = 11 + 143*msgStruct->curMsgNumber;

	char *buffer = (char *) mymalloc( (sizeofBuffer + 1) * sizeof(char));

	strcpy(buffer, "SMESSAGES\n");

	char str[10] = "";
	char *ptr = NULL;

	struct msgList *node = msgStruct->head;

	int nleft = 0;
	int nwritten = 0;

	// ----juntar a mensagesm SMESSAGES---- 
	addMessages(str, buffer, node, flag);

	strcat(buffer, "\n");

	// ----- envio da mensagem ----------
	nleft = strlen(buffer);
	ptr = buffer;

	//printf("buffer no sendMessages-> %s\n", buffer); // Debug

	while(nleft > 0) {

		//printf("no ciclo do write-> nleft: %d\n", nleft);  // Debug

		nwritten = write(fd, ptr, nleft);
		if(nwritten <= 0) {
			printf("Error writing in sendMessages. Connection may be lost:%s\n", strerror(errno) );
			// fechar o file descriptor
			FD_CLR(fd, rfds);
			removeServer(&(myServer->head), fd);
			myServer->n_servers--;
			return -1;
		}
		nleft -= nwritten;
		ptr += nwritten;
	}	

	free(buffer);

	return 1;
}



// envia um SGET_MESSAGES PARA UM SERVIDOR ALEATORIAMENTE
void sendMsgRequest(struct servers_info *myServer, fd_set *rfds, int *flgReadState) {

	struct servers_list *temp = myServer->head;

	char buffer[15] = "SGET_MESSAGES\n";
	char *ptr = buffer;

	int nleft = 0;
	int nwritten = 0;

	int iterations = 0;	// o nº de iteraçoes corresponde ao nº do server
	int success = 0;	// se lemos tudo o que era supostoe

	nleft = strlen(buffer);

	int n_random = 0; 
	if(myServer->n_servers > 1) {
		n_random = rand() % myServer->n_servers;
	}

	// OBJETIVO É ESCOLHER UM SERVER RANDOMLY PARA PEDIR AS SUAS MENSAGENS 
	while(success != 1) {

		while(iterations != n_random){
			// se o n_random for 0, não se entra no ciclo. senão for ele corre até chegar a iterations = n_random
			temp = temp->next;
			iterations++;
		}

		while(nleft > 0) {
			nwritten = write(temp->fd_tcp, ptr, nleft);
			if(nwritten <= 0) {
				printf("Error writing in sendMsgRequest. Connection may be lost:%s\n", strerror(errno) );
				// fechar o file descriptor
				FD_CLR(temp->fd_tcp, rfds);
				removeServer(&(myServer->head), temp->fd_tcp);
				myServer->n_servers--;
				// escolhe outro servidor ao qual enviar
				n_random = 0;
				if(myServer->n_servers > 1) {
					n_random = rand() % myServer->n_servers;
				}
				ptr = buffer;
				nleft = strlen(buffer);
				temp = myServer->head;
				break;
			}

			nleft -= nwritten;
			ptr += nwritten;
		}	

		if(nleft == 0){
			success = 1;	// para sairmos do ciclo
			*flgReadState = temp->fd_tcp;	// para dizer que enviamos o pedido metemos a flag >0 e com o valor correspondente ao valor do fd para depois sabermos qual era o server a qual pedimos 
		}

	}

	return;
}

// lê o que um certo servidor de mensagens enviou
int attendTCPserver(struct servers_list * server, char buffer[AWS_SIZE], struct servers_info *myServer, fd_set *rfds, int *flgReadState) {

	int nread = -2;

	char * ptr = buffer;

	int nleft = AWS_SIZE;

	// lemos coisas enviadas por TCP
	nread = read(server->fd_tcp, ptr, nleft);

	//write(1, buffer, AWS_SIZE); 					// Debug
	//printf("valor do nread: %d\n", nread);

	// o servidor retornou -1, significa que perdemos a ligação de alguma maneira
	if(nread == -1) {
		
		//printf("error in read:%s\n", strerror(errno)); // Debug

		// conexão com o outro servidor foi perdida! 
		if(server->fd_tcp == *flgReadState){
			*flgReadState = 0;
			// Temos que escolher um novo servidor e enviar-lhe um novo pedido
			if(myServer->n_servers != 0){
				sendMsgRequest(myServer, rfds, flgReadState);
			}
		}

		// removemos o servidor
		FD_CLR(server->fd_tcp, rfds);
		removeServer(&(myServer->head), server->fd_tcp);
		myServer->n_servers--;
		return -1;
	}
	// o servidor do outro lado disconectou-se -> retiramo-lo da lista
	else if(nread==0){
		FD_CLR(server->fd_tcp, rfds);
		removeServer(&(myServer->head), server->fd_tcp);
		myServer->n_servers--;
		return -1;
	}

	// se o buffer não acabar em \n\n significa que ainda vai ser necessário ler mais alguma coisa
	if(buffer[nread - 1] != '\n' && buffer[nread - 2] != '\n') {
		buffer[nread] = '\0';

		//printf("A mensagem ainda não acabou, é necessário continuar a ler!\n");		// Debug

		// flag que significa que a próxima mensagem não vai começar com SMESSAGES
		server->flagMsg = 1;
	}

	return 1;
}

//percorre a lista de servidores de mensagens à procura de um cujo FD esteja pronto a ser escrito e aí ler o que tem a dizer
void waitingTcpOperations(struct msgs *msgStruct, fd_set *rfds, struct servers_info *myServer, int *flgReadState) {

	char buffer[AWS_SIZE];
	memset(buffer, '\0', AWS_SIZE);

	struct servers_list * temp = myServer->head;
	struct servers_list * auxiliar = NULL;

	
	while(temp != NULL) {

		// testa o estado do fd de todos os servidores guardados na lista
		if( FD_ISSET(temp->fd_tcp, rfds) ) {

			memset(buffer, '\0', sizeof(buffer));
			
			//printf("Request for attention: %s port: %hu socket:%d\n", inet_ntoa(temp->addr.sin_addr), ntohs(temp->addr.sin_port), temp->fd_tcp );		//Debug

			auxiliar = temp->next;

			// ler mensagem. Se tiver sido bem recebido, então vê que tipo de operação foi pedida
			if( attendTCPserver(temp, buffer, myServer, rfds, flgReadState) == 1) {
				// perceber se é um SGET_MESSAGES\n, um SMESSAGES\n ou uma mensagem
				checkTcpOperation(temp->fd_tcp, buffer, myServer, msgStruct, rfds, temp);
				memset(buffer, '\0', sizeof(buffer));
				temp = auxiliar;
			}
			else {
				temp = auxiliar;
			}		

		}
		else{
			temp = temp->next;			
		}
	}

	return;
}


void checkTcpOperation(int fd, char buffer[AWS_SIZE], struct servers_info *myServer, struct msgs *msgStruct, fd_set *rfds, struct servers_list * server) {

	char auxiliar[AWS_SIZE];
	memset(auxiliar, '\0', AWS_SIZE);

	strcpy(auxiliar, buffer);
	char *token = strtok(buffer, "\n");
	int flag = -1; // significa que é para mandar todas as mensagens

	if (token == NULL)
		return;

	// vê se foi do tipo SMESSAGES
	if(!strcmp(token, "SMESSAGES")){
		receiveMsgList(msgStruct, auxiliar, server);
	}
	// vê se foi do tipo SGET_MESSAGES
	else if(!strcmp(token, "SGET_MESSAGES") ){
		sendMessages(msgStruct, fd, myServer, rfds, flag);
	}
	// Significa que a mensagem foi partida porque excedeu o buffer do Read
	else if ( strcmp(token, "SMESSAGES") && strcmp(token, "SGET_MESSAGES") && server->flagMsg == 1 ){
	
		// Função que lê funções que começam incompletas
		receiveIncompleteMsgList (msgStruct, auxiliar, server);
	}

	return;
}


// liberta a memória alocada para a lista dos servidores de mensagens e fecha as sockets utilizadas
void freeServersList(struct servers_info *myServer, fd_set *rfds){

	struct servers_list *auxiliar = NULL;

	while(myServer->head != NULL) {
		FD_CLR(myServer->head->fd_tcp, rfds);
		close(myServer->head->fd_tcp);
		auxiliar = myServer->head;
		myServer->head = myServer->head->next;
		free(auxiliar);

	}

	return;
}


// envia todas as mensagens que possui para um ou mais servidores
void shareMsg(struct msgs *msgStruct, struct servers_info *myServer, fd_set *rfds) {

	struct servers_list *temp = myServer->head;
	struct servers_list *auxiliar = NULL;

	int flag = 1; // a flag igual a 1, significa que só queremos mandar uma mensagem

	while(temp != NULL) {

		auxiliar = temp->next;

		if( sendMessages(msgStruct, temp->fd_tcp, myServer, rfds, flag) == -1) {
			printf("The message wasn't successfully sent.\n");
		}

		temp = auxiliar;
	}

	return;
}


// Recebe a string SGETMESSAGES e insere as mensagens e o LC na lista de mensagens
void receiveMsgList( struct msgs *msgStruct, char buffer[AWS_SIZE], struct servers_list * server) {

	char delimiter[2] = "\n";
	char *token = NULL;

	int LC = -1;					// Logic Clock
	char msg[1024] = ""; 			// mensagem 

	char aux[180] = "";

	int counter = 0;	// porque a primeira vez que é feito o ciclo é especial

	token = strtok(buffer, delimiter);

	if(token == NULL) {
		return;
	}

	// RACIOCINO: lê se o token e guarda-se o resultado no aux. Volta-se a ler o token. Se o token for NULL significa que
	// a parte incompleta está no aux, senão for NULL, então inserimos os dados do aux na lista e o aux passa a ser o novo token
	while (token != NULL){

		token = strtok(NULL, delimiter);

		//printf("token -> %s,,, aux -> %s\n", token, aux); // Debug

		// A mensagem foi mal finalizada, guarda-se num buffer a ultima mensagem incompleta
		if(token == NULL && counter != 0 && server->flagMsg == 1) {

			// gaurda-se a parte incompleta num buffer de modo a ser completada quando se receber o resto
			strcpy(server->auxstr, aux);

			break;
		}
		// A mensagem foi bem finalizada
		else if(token == NULL && server->flagMsg == 0) {

			if(sscanf(aux,"%d;%[^\n]s", &LC, msg) == 2){

				// para garantir que a mensagem que é enviada não excede os 140 caracteres
				msg[msgSize] = '\0';

				//printf("messages -> %s,,,, LC -> %d\n", msg, LC);    // Debug

				msgStruct->curMsgNumber += insertMsgList(msg, &msgStruct->head, msgStruct, LC);

				msgStruct->LC = max(LC, msgStruct->LC) + 1;

				break;
			}
		}
		// No caso de ser lido um SMESSAGES então ignoramo-lo
		else if(!strcmp(token, "SMESSAGES")){
			continue;
		}
		// insere-se o antigo token na lista (o aux) e o aux para a ser o token de agora
		else if (token != NULL && counter != 0){

			if(sscanf(aux,"%d;%[^\n]s", &LC, msg) == 2){

				// para garantir que a mensagem que é enviada não excede os 140 caracteres
				msg[msgSize] = '\0';

				//printf("messages -> %s,,,, LC -> %d\n", msg, LC);    // Debug

				msgStruct->curMsgNumber += insertMsgList(msg, &msgStruct->head, msgStruct, LC);

				msgStruct->LC = max(LC, msgStruct->LC) + 1;

				memset(aux, '\0', 143);
				strcpy(aux, token);
			}	
			
		}
		// Neste caso o aux ainda não tinha nada para inserir na lista, então não se faz
		else if (token != NULL && counter == 0){

			memset(aux, '\0', 143);

			strcpy(aux, token);
		}

		counter++;
	}

	return;
}



// serve para receber mensagens que não começam por SMESSAGES ou SGET_MESSAGES porque são a continuação de uma mensagem anterior
void receiveIncompleteMsgList (struct msgs *msgStruct, char buffer[AWS_SIZE], struct servers_list * server) {

	char delimiter[2] = "\n";
	char *token = NULL;

	//char LC[10] = "";  			
	int LC = -1;					// Logic Clock
	char msg[1024] = ""; 			// mensagem 

	char aux[160] = "";

	int counter = 0;		// porque a primeira vez que é feito o ciclo é especial

	token = strtok(buffer, delimiter);

	if(token == NULL) {
		return;
	}
	else {
		// junta a sobra da mensagem incompleta à sobra do resto dessa mensagem
		strcat(server->auxstr, token);

		// adiciona na lista
		if(sscanf(server->auxstr, "%d;%[^\n]s" , &LC, msg) == 2){
			msg[msgSize] = '\0';
			msgStruct->curMsgNumber += insertMsgList(msg, &msgStruct->head, msgStruct, LC);
			msgStruct->LC = max(LC, msgStruct->LC) + 1;
		}
		// limpa o buffer porque já não vai ser usado para esta mensagem
		memset(server->auxstr, '\0', 160);
	}

	while(token != NULL) {

		token = strtok(NULL, delimiter);

		//printf("token -> %s,,, aux -> %s\n", token, aux);		// Debug

		// A mensagem foi mal finalizada, guarda-se num buffer a ultima mensagem incompleta
		if(token == NULL && counter != 0 && server->flagMsg == 1) {

			// gaurda-se a parte incompleta num buffer de modo a ser completada quando se receber o resto
			strcpy(server->auxstr, aux);

			break;
		}
		// A mensagem foi bem finalizada
		else if(token == NULL && server->flagMsg == 0) {

			if( sscanf(aux,"%d;%[^\n]s", &LC, msg) == 2) {

				// para garantir que a mensagem que é enviada não excede os 140 caracteres
				msg[msgSize] = '\0';

				//printf("messages -> %s,,,, LC -> %d\n", msg, LC);		// Debug

				msgStruct->curMsgNumber += insertMsgList(msg, &msgStruct->head, msgStruct, LC);

				msgStruct->LC = max(LC, msgStruct->LC) + 1;

				break;
			}
		}
		// insere-se o antigo token na lista (o aux) e o aux para a ser o token de agora
		else if (token != NULL && counter != 0){

			if( sscanf(aux,"%d;%140[^\n]s", &LC, msg) == 2) {

				// para garantir que a mensagem que é enviada não excede os 140 caracteres
				msg[msgSize] = '\0';

				// printf("messages -> %s,,,, LC -> %d\n", msg, LC);    // Debug

				msgStruct->curMsgNumber += insertMsgList(msg, &msgStruct->head, msgStruct, LC);

				msgStruct->LC = max(LC, msgStruct->LC) + 1;

				memset(aux, '\0', 143);
				strcpy(aux, token);
			}
		}
		// Neste caso o aux ainda não tinha nada para inserir na lista, então não se faz
		else if (token != NULL && counter == 0){

			memset(aux, '\0', 143);

			strcpy(aux, token);
		}

		counter++;
	}


	return;
}