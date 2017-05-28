#include "interface.h"

void interface(struct sockaddr_in *id_server, struct msg_id * msgS , int reg_int, int max_msg){

	char input[160];  //tem que ser o tamanho da expressão 1 mais o tamanho da mensagem
	char opt1[20];		
	char opt2[msgSize];

	time_t sec_ant;	// tempo mal saimos do select
	time_t sec_now;	// tempo a chegar ao select
	int yes = 1 ;	// ???


	// FLAGS ----------------
	int flgJoin = 0;			// flag para saber se o join já foi feito a primeira vez ou não
	int flgReadingState = 0;	// flag para ver se der erro em servidor temos que tentar reenviar

	// ----------------------
	char buffer[BUFFER_SIZE] = "";
	char temp_str[20] = "";

	// tamanhos de Sockaddr_in
	socklen_t addrlenUDP = 0, addrlenTCP = 0;

	// declaração de file descriptors e coisas relacionadas
	int fd_clientUDP = -1, newfd = -1, fdID = -1;
	int fdmax = 0, counter = 0, n = 0;
	fd_set rfds;

	// estruturas sockaddr_in que vamos utilizar
	struct sockaddr_in udp_gate, tcp_gate, tcpClient;

	// timer do select
	struct timeval tv;

	// declaração e inicialização da estrutura onde se guarda as mensagens e outros
	struct msgs msgStruct;		//estrutura principal de mensagens
	msgStruct.head = NULL; 		//struct msgList
	msgStruct.curMsgNumber = 0;
	msgStruct.maxMsgNumber = max_msg;
	msgStruct.LC = 0;

	// declaração e inicialização estrutura que guarda a lista de servidores
	struct servers_info myServer;
	myServer.n_servers = 0;
	myServer.fd_link = -1;
	myServer.head = NULL;


	//////UDP///////////////

	// inicializações
	setup_sockets(&fdID, &fdmax ,AF_INET,SOCK_DGRAM);	
	setup_sockets(&myServer.fd_link, &fdmax ,AF_INET,SOCK_STREAM);
	setup_sockets(&fd_clientUDP, &fdmax ,AF_INET,SOCK_DGRAM);

	memset( (void*) &(udp_gate), (int)'\0', sizeof(udp_gate));

	udp_gate.sin_family = AF_INET;
	udp_gate.sin_addr.s_addr=htonl(INADDR_ANY);
	udp_gate.sin_port = msgS->term_port; /////port

	// BIND

	n=bind(fd_clientUDP,(struct sockaddr *)&(udp_gate),(socklen_t)sizeof(udp_gate));
	if(n<0){
		printf("erro bind: %s\n",strerror(errno));
		exit(1);
	}

	addrlenUDP = sizeof(udp_gate);

	//////TCP///////////////

	setsockopt(myServer.fd_link,SOL_SOCKET,SO_REUSEADDR, &yes, sizeof(int));

	// inicializações 
	memset((void*)&(tcp_gate), (int)'\0', sizeof(tcp_gate));
	tcp_gate.sin_family = AF_INET;
	tcp_gate.sin_addr.s_addr=htonl(INADDR_ANY);
	tcp_gate.sin_port = msgS->serv_port; /////port

	addrlenUDP = sizeof(tcp_gate);

	// BIND
	if( bind(myServer.fd_link,(struct sockaddr*) &tcp_gate, sizeof(tcp_gate))==-1){
		printf("bind:%s\n", strerror(errno));
		exit(1);
	}

	// LISTEN
	if( listen(myServer.fd_link,10) == -1){
		printf("listen:%s\n", strerror(errno));
		exit(1);
	}
	
	memset( (void*) &(tcpClient), (int)'\0', sizeof(tcpClient));

	// Input Option
	PROMPT;

	// O WHILE
	while(1) {

		FD_ZERO(&rfds);
		FD_SET(fd_clientUDP, &rfds);
		FD_SET(myServer.fd_link, &rfds);
		FD_SET(STDIN, &rfds);
		fdSET(myServer.head, &rfds);

		// Cálculo do valor do timer
		if(flgJoin == 1){
			sec_now = time(NULL);
			if(sec_now - sec_ant >= reg_int){
				tv.tv_sec = 0;
			}
			else{
				tv.tv_sec = reg_int-(sec_now - sec_ant);
			}
			tv.tv_usec = 0;
		}


		// Há 2 situações: uma em que ainda não foi feito o 1º join e o timer ainda não foi inicializado e outra em que já foi e o timer é de r segundos
		if(flgJoin == 0){
			counter=select(fdmax + 1, &rfds, (fd_set*)NULL,(fd_set*)NULL, (struct timeval*)NULL);
		}
		else
			counter=select(fdmax + 1, &rfds, (fd_set*)NULL,(fd_set*)NULL, &tv);
		
		if (counter < 0){
			printf("Erro no select: %s\n", strerror(errno));
			exit(1);
		}

		// Cálcula as horas logo à saida do select
		sec_ant = time(NULL);

		// se o timer for excedido então counter == 0 e o servidor regista-se automaticamente
		if(counter == 0){
			join(id_server, msgS, fdID);
		}


		/******************************************************************/
		/* 		SITUAÇÃO EM QUE O SERVIDOR RECEBE MENSAGENS POR TCP       */
		/******************************************************************/

		if (FD_ISSET(fd_clientUDP, &rfds)){

			// limpa o buffer antes de o utilizar
			memset(buffer,'\0', BUFFER_SIZE);

			n = recvfrom(fd_clientUDP, buffer, BUFFER_SIZE, 0 , (struct sockaddr *)&udp_gate , &addrlenUDP);
			if( n == -1){
				printf("error in recvfrom of get_message %s\n", strerror(errno));
				exit(1);
			}
			
			//write(1, buffer, BUFFER_SIZE);  	// Debug
			if(buffer[0] == 'P'){
				printf("\n");						// Debug
				PING;
				printf("\n");	
				PROMPT;
			}
			//faz a publicação e a remoção de mensagens
			//responde tambem
			//<param>answer</param> contem a resposta a ser dada
			bufferAnalyser(buffer, (struct msgs *)&msgStruct, fd_clientUDP, (struct sockaddr *)&udp_gate, &myServer, &rfds);

			counter--;
		}


		/******************************************************************/
		/* 	         PARA LER PEDIDOS DE CONEXÃO PELO ACCEPT              */
		/******************************************************************/

		if (FD_ISSET(myServer.fd_link, &rfds)) {

			// recebe pedidos de conexão e retorna um fd da ligação. guardamos também os dados do servidor na lista
			newfd = accept(myServer.fd_link, (struct sockaddr*)&tcpClient, &addrlenTCP);
			
			// se o accept for mal feito o newfd é inicializado a -1. A função getpeername é utlizada para preencher corretamente a estrutura
			// uma vez que em certos casos o ip não fica com valores
			if( newfd != -1  && !getpeername( newfd, (struct sockaddr*)&tcpClient, &addrlenTCP)) {

				//printf("Aceitei alguma conexão\n"); 	//Debug

				fdmax = max(fdmax, newfd);

				// transforma o unsigned short para char para ser usado no fill_servers_list
				sprintf(temp_str, "%hu", ntohs(tcpClient.sin_port));

				// adiciona o servidor à lista de servidores
				fill_servers_list(&myServer, inet_ntoa(tcpClient.sin_addr) , temp_str, newfd );
			}

			counter--;

		}

		//******************************************************************/
		/* 	        		PARA LER COMANDOS DO TERMINAL			       */
		/*******************************************************************/
		if (FD_ISSET(STDIN, &rfds))
		{

			if (fgets(input, 160 , stdin) == NULL){
				printf("erro na linha \n");
			}

			if(sscanf(input,"%s %s", opt1, opt2) == 0){
				printf("erro na separação\n");
			}


			if(strcmp(opt1, "join") == 0 && !FD_ISSET(myServer.fd_link, &rfds)){
				printf(">>>joining the identities server:\n");

				// regista-se o servidor de mensagens no servidor de identidades
				join(id_server, msgS, fdID);

				// chamar get_servers
				if( flgJoin == 0) {
					// ver que servidores estão conectados ao servidor de identidades e guarda-os numa lista
					get_servers(id_server, &myServer, msgS);
					//função de connectar com os servidores de mensagens
					make_connections(&myServer, &rfds, &fdmax);

					// envia um pedido para receber todas as mensagens
					if( myServer.n_servers != 0)
						sendMsgRequest(&myServer, &rfds, &flgReadingState);
				}

				// a partir de agora o timer inicia
				flgJoin = 1;

				memset(opt1, '\0', 20);
				memset(opt2, '\0', msgSize);

				// Input Option
				PROMPT;
			}
			else if(strcmp(opt1, "show_servers") == 0 && !FD_ISSET(myServer.fd_link, &rfds)){
				printf(">>>showing other message servers:\n");
				
				// mostra os servidores com que há uma conexão TCP 
				showServers(myServer);

				// Input Option
				PROMPT;
			}

			else if(strcmp(opt1, "show_messages")== 0){
				printf(">>>showing messages:\n");
				// chamar show_messages
				showMessages(msgStruct.head);

				// Input Option
				PROMPT;
			}
			else if(strcmp(opt1, "exit") == 0){

				// dá free das coisas e remove os servidores e mensagens da lista
				FD_CLR(myServer.fd_link, &rfds);
				close(myServer.fd_link);
				freeMsgList(&msgStruct);
				freeServersList(&myServer, &rfds);
				FD_CLR(fd_clientUDP, &rfds);
				close(fd_clientUDP);
				return;
			}
			// se nada do que foi escrito corresponde ao suposto
			else{
				printf("Invalid input, try:\n");
				printf("\tjoin\n\tshow_servers\n\tshow_messages\n\texit\n\n");
			}

			counter--; 
		}

		// Se ainda houver file descriptors prontos a serem utilizados
		if (counter > 0) 
			waitingTcpOperations( &msgStruct, &rfds, &myServer, &flgReadingState);
		

	}



}