#ifndef UTILSRMB_C
#define UTILSRMB_C

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>

#define lineSize 128  // CHANGE
#define msgSize 141   //CHANGE
#define BUFFER_SIZE 2048
#define MSG_BUFFER_SIZE 1024
#define max(A,B) ((A)>=(B)?(A):(B))
#define STDIN 0
#define SHOW_SERVERS 1
#define PUBLISH 2
#define SHOW_LATEST_MESSAGES 3
#define TIMEOUT 1
#define MAX_SERVER 100

#define DOT "\x1b[38;5;220m . \x1b[0m"
#define WAITING write(1, DOT, strlen(DOT))
#define SYMBOL "\x1b[38;5;118m$ \x1b[0m"
#define PROMPT write(1, SYMBOL, strlen(SYMBOL))

extern int h_errno; /* Para a função gethostbyname */

extern int errno;

int bufferMessagesSize; // declaramos como variavel global para não termos que passar por referência para 10 funções

// informação de cada operação da lista
struct opt2_list {
	struct opt2_list * next;
	char opt[141];		// operação propriamente dita que será feita
	int optype; // show_servers = 1, publish = 2 e show_latest_messages = 3
};

// Informação geral da estrutura de operações
struct opt2_info{
	struct opt2_list * head;
	int listSize;	// nº de mensagens guardadas
};

// estrutura que guarda várias flag utilizadas ao longo do programa
struct Flags {
	int publish;		// um publish está a decorrer
	int showServers;	// um showServers está a decorrer
	int showLatMsg;		// um showLatestMessages está a decorrer
	int timerIDServer;	// o timer da conexão ao servidor de identidades está a ser utilizado
	int msgTest;	// está a ser testado se a mensagem enviada foi recebida
};

// Lista de servidores bloqueados, ou seja, tentamos conectar a eles e não foi possivel. assim já não voltamos a tentar
struct Blocked
{
	struct Blocked *next;
	struct sockaddr_in id;	// ip e porto do servidor
	int counter;			// nº de vezes que tentámos conectar a ele
	int existence;			// nº auxiliar para ver se o servidor bloqueado se encontra de momento no servidor de identidades
};

// estrutura geral que guarda servidores
struct serverStruct{
	socklen_t addrlen;		// tamanho do theChosenOne
	struct sockaddr_in theChosenOne;	// servidor com o qual estamos a partilhar e receber info no momento. Vai ser alterado à medida que o server se disconecta
	struct sockaddr_in serv[MAX_SERVER];	// tabela onde guardamos os servidores que estamos no momento do servidor de identidades
	struct Blocked *head;	// head para a lista de servers bloqueados
	int numBlocked;			// nº de servidores bloqueados
	int fd;					// file descriptor utilizados para contactar servidores de mensagens
	int n_servers;			// nº de servidores presentes no serv
};

int sendToServer(int fd, struct sockaddr_in *gate, socklen_t * addrlen, char *message);

void setSockaddr( struct sockaddr_in * server, socklen_t *addrlen, struct in_addr * addr, unsigned short port);

void sigPipe();

void setup_sockets(int *fd, int *fdmax);

void * mymalloc(int size);

void printOpt(struct opt2_list * node);

void insertOpt(char buffer[141],int type, struct opt2_info * opt2info);

void removeOpt(struct opt2_info * opt2info);

int compareServers(struct sockaddr_in new, struct sockaddr_in server);

void freeOptList(struct opt2_info * opt2info, int flag);

int checkMsgPlace(char buffer[MSG_BUFFER_SIZE], struct opt2_info * opt2info);

void initBlocked(struct serverStruct *servStruct);

/// Retorna 1 se NEW estiver Bloqueado
int checkBlocked(struct sockaddr_in new, struct serverStruct  * ServStruct, int flag);

// remove um servidor que deixou de estar registado no servidor de identidades
void resetBlocked(struct serverStruct * ServStruct);

void insertBlocked (struct serverStruct *ServStruct, struct sockaddr_in blocked);

void freeBlocked (struct Blocked **aux);

void genServer(struct serverStruct * ServStruct);

#endif