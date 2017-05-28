/*****************************************************************************/
/*               Projeto de RCI - Reliable Message Board                     */
/*                   Manuel Reis & Renato Henriques                          */
/*****************************************************************************/

#include "readArgs.h"
#include "user_cmds.h"
#include "handleOpt.h"


/* cada servidor de mensagens é definido pelo seu nome,
pelo seu endereço IPv4, o porto ao qual ele aceita pedidos
vindo de terminais e o porto ao qual ele aceita pedidos vindo
de outros servidores de mensagens */ 

void interface(struct  sockaddr_in * id_server, socklen_t * addrlenID){

	char buffer[BUFFER_SIZE] = "";
	char publishBuffer[180] = "PUBLISH ";
	char showLatMsgBuffer[50] = "GET_MESSAGES ";
	char *bufferMessages = NULL;
	int testeReadCmds = -2;

	/*******************
	* File Descriptors *
	*******************/
	fd_set rfds;
	int fdID = -1;
	int fdmax= -1;
		
	setup_sockets(&fdID, &fdmax);

	int counter = -1;

	/*******************
	* Pedidos a tratar *
	*******************/
	struct Flags flags;
	flags.publish = 0;
	flags.showServers = 0;
	flags.showLatMsg = 0;
	flags.timerIDServer = -1;
	flags.msgTest = 0;

	struct opt2_info opt2info;
	opt2info.head = NULL;
	opt2info.listSize = 0;


	// Inicializada a estrutura que define o tempo de timeout associado ao select
	struct timeval tv;
	tv.tv_sec = TIMEOUT;
	tv.tv_usec = 0;

	/***************************
	* Inicialização de structs *
	***************************/

	// Declaração da estrutura de Servidores de Mensagens
	struct serverStruct ServStruct;
	ServStruct.addrlen = 0;
	// Inicialização do File Descriptor a usar
	ServStruct.fd = -1;
	//Ponteiro para lista de servidores bloqueados
	ServStruct.head = NULL;
	ServStruct.numBlocked = 0;
	// inicializa a socket utilizada para comunicar com o chosen one
	setup_sockets(&(ServStruct.fd), &fdmax);

	// A variavel que representa o servidor escolhido para comunicar é inicializado com um Ip não valido
	// um a vez que ainda nao foi escolhido
	inet_aton("0.0.0.0", &(ServStruct.theChosenOne.sin_addr));
	// É preparada uma socket para comunicar com esse servidor de mensagens
	setSockaddr(&(ServStruct.theChosenOne), &(ServStruct.addrlen), &(ServStruct.theChosenOne.sin_addr), 0);

	/************
	* Main Loop *
	************/

	//É mostrada a prompt
	PROMPT;

	while(1){

		FD_ZERO(&rfds);
		FD_SET(fdID, &rfds);
		FD_SET(STDIN, &rfds);
		FD_SET(ServStruct.fd, &rfds);

		tv.tv_sec = TIMEOUT;

		counter = select(fdmax + 1 , &rfds, (fd_set *)NULL, (fd_set *)NULL, &tv);


		// quando se dá o timeout 
		// se ninguem nos respondeu ( counter = 0 )
		if(counter == 0){
			
			// Se tivermos tentado contactar o servidor de identidades
			// mais de 3 vezes, sem obter resposta. Iremos abortar.
			if(flags.timerIDServer > 3) {
				printf("Can't connect to identity Server. Aborting program.\n");

				// dar free de tudo
				close(ServStruct.fd);
				close(fdID);
				freeBlocked(&(ServStruct.head));
				freeOptList(&opt2info, 0);

				exit(1);
			}

			// Caso seja menos de 3 vezes
			if(flags.timerIDServer >= 0 ){

				// significa que ainda nao obtivemos resposta
				flags.timerIDServer++;
				// voltamos a tentar contactar
				if( sendToServer(fdID, id_server, addrlenID, "GET_SERVERS") == -1){
					printf("Can't connect to identity Server.\n");
					// dar free de tudo o que temos alocado
					freeBlocked(&(ServStruct.head));
					freeOptList(&opt2info, 0);

					exit(1);
				}
			}

			/////////////////////////////////////////////////////////////////////////////////////

			// Se não houver resposta do servidor de mensagens
			// e temos pedidos pendentes
			if( flags.showLatMsg > 0 || flags.msgTest > 0) { // Usando o timer de 2 segundos também para o recvfrom do show_latest_messages

				// se não tiver excedido o limite de tentativas
				// volta a pedir os servidores o que incadeia de novo o processo
				if( counterExceeded( &flags, &ServStruct, fdID, addrlenID, id_server, &opt2info) == -1) {

						if( sendToServer(fdID, id_server, addrlenID, "GET_SERVERS") == -1){
							printf("Can't connect to identity Server.\n");

							// dar free de tudo
							if(bufferMessages != NULL){
								free(bufferMessages);
								bufferMessages = NULL;
							}
							close(ServStruct.fd);
							close(fdID);
							freeBlocked(&(ServStruct.head));
							freeOptList(&opt2info, 0);

							exit(1);
						}
				}
			}			
		}
		
		/**********************************************************
		* TRATAMENTO DE COMUNICAÇÂO COM O SERVIDOR DE IDENTIDADES *
		**********************************************************/
		if(FD_ISSET(fdID, &rfds) && opt2info.head != NULL){

			// reset do timer do id server,
			// obteve-se resposta
			flags.timerIDServer = -1;

			// À partida o SID terá respondido com a lista de servidores,
			// vamos analisar:
			getServers(fdID, id_server, addrlenID, &ServStruct, buffer, &fdmax, &flags, &opt2info, &bufferMessages);
			
			// descobre que operação devo fazer a seguir com base no que foi lido do servidor de identidades
			handleOpt(&flags, &ServStruct, &opt2info, buffer, publishBuffer, showLatMsgBuffer, fdID, addrlenID, id_server, &bufferMessages);

		}

		/********************************************************
		* TRATAMENTO DE COMUNICAÇÂO COM SERVIDORES DE MENSAGENS *
		********************************************************/

		if(FD_ISSET(ServStruct.fd, &rfds)) {

			// receber o que o servidor de mensagens tem para dizer
			if (recvfrom(ServStruct.fd, bufferMessages, bufferMessagesSize, 0, (struct sockaddr*) &(ServStruct.theChosenOne), &(ServStruct.addrlen)) == -1) {
				printf("error recvfrom of msg serv: %s\n", strerror(errno) );
			}

			//para debug: printf("BUFFERMESSAGES à saida do recvfrom: %s\n", bufferMessages);

			// se estivermos à espera da confirmação de uma mensagem 
			if(flags.msgTest > 0 && opt2info.head->optype != SHOW_LATEST_MESSAGES){

				// verificamos se o numero de tentattivas expirou
				if( counterExceeded( &flags, &ServStruct, fdID, addrlenID, id_server, &opt2info) == 1 ) {

				}

				// caso não tenha expirado
				// a mensagem enviada é procurada na resposta obtida
				else if( checkMsgPlace(bufferMessages, &opt2info) == 1){

					printf("\nThe messagem (%s) was successfully received by the message server.\n", opt2info.head->opt); // <--------------------------------------
					PROMPT;
					// dá free do bufferMessages para o seu tamanho dar reset
					if(bufferMessages !=NULL){
						free(bufferMessages);
						bufferMessages =NULL;
					}

					// finalmente termina o pedido de publish
					flags.publish--;
					flags.msgTest = 0;
					// remove-se a mensagem da lista
					removeOpt(&opt2info);

					// limpa o buffer
					memset(publishBuffer, '\0', 180);
				}

				// se a mensagem não estiver entre a resposta
				else {
					// reenvio a mensagem para o servidor de mensagens escolhido
					if( sendToServer(ServStruct.fd, &(ServStruct.theChosenOne), &(ServStruct.addrlen), publishBuffer) == 1){

						WAITING;
						//printf("reenviei a mensagem: %s : para %s com port %hu\n", publishBuffer, inet_ntoa(ServStruct.theChosenOne.sin_addr), ntohs(ServStruct.theChosenOne.sin_port));

						// chama-se o show_latest_messages para verificarmos se a mensagem foi enviada com sucesso
						// 5 é um nº escolhido, pode ter que ser alterado 
						memset(showLatMsgBuffer, '\0', 50);

						strcpy(showLatMsgBuffer, "GET_MESSAGES 5");

						bufferMessagesSize = (12 + 140*5);

						if(bufferMessages !=NULL){
							free(bufferMessages);
							bufferMessages =NULL;
						}

						bufferMessages = (char *) mymalloc( bufferMessagesSize *sizeof(char));

				//printf("Aloquei memória para o bufferMessages: %s\n", bufferMessages);

						sendToServer(ServStruct.fd, &(ServStruct.theChosenOne), &(ServStruct.addrlen), showLatMsgBuffer);

						// a flag para indicar que vamos aguardar uma confirmação
						flags.msgTest++;
					}
					else
						flags.msgTest++;

				}


			}
			// Se estivermos espera de receber a resposta a um show_latest_messages
			else if(flags.showLatMsg > 0) {

				flags.msgTest = 0;

				// imprime-se as mensagens 
				printf("\n%s\n", bufferMessages);
				PROMPT;

				// dá free do bufferMessages para o seu tamanho dar reset
				if(bufferMessages !=NULL){
					free(bufferMessages);
					bufferMessages =NULL;
				}

				// finalmente termina-se a operação de mostrar as mensagens
				flags.showLatMsg--;

				// remove-se o nº de mensagens da lista
				removeOpt(&opt2info);
			}

			///////////////////////////////////////////

		}

		/****************************************************
		*	 TRATAMENTO DE COMUNICAÇÂO COM O TECLADO		*
		****************************************************/
		if(FD_ISSET(STDIN, &rfds)){

			// se o utilizador quiser sair testeReadCmds = 1
			if( (testeReadCmds = readCmds(&flags, &opt2info, &ServStruct, fdID)) == -1){
				close(ServStruct.fd);
				close(fdID);
				if(ServStruct.head!=NULL)
					freeBlocked(&(ServStruct.head));
				freeOptList(&opt2info, 0);

				break;
			}
			else if( testeReadCmds == 1) {
				//significa que utilizador nao saiu então vou pedir os servidores
				if( sendToServer(fdID, id_server, addrlenID, "GET_SERVERS") == -1){
					printf("Can't connect to identity Server.\n");

					// dar free de tudo
					close(ServStruct.fd);
					close(fdID);
					freeBlocked(&(ServStruct.head));
					freeOptList(&opt2info, 0);

					exit(1);
				}

				flags.timerIDServer = 0;
			}
		}	
	}
}


int main(int argc, char const *argv[])
{
	struct sockaddr_in id_server;
	socklen_t addrlenID = 0;
	sigPipe();

	// gerar seeds random
	struct timeval t1;
	gettimeofday(&t1, NULL);
	srand(t1.tv_usec * t1.tv_sec); // para ter uma diferente seed a cada inicio do programa

	/* predefine o valor do ip e do porto do servidor de identidades com os valores default */

	setDefaultSettings( (struct sockaddr_in *) &id_server, &addrlenID);

	/* lê os argumentos do rmb, nomeadamente, o IPv4 e o porto de acesso */

	read_args(argc, argv, (struct sockaddr_in *) &id_server);


	// Main Funcion
	interface(&id_server, &addrlenID);
		
	return 0;
}