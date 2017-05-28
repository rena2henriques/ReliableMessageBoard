#ifndef UTILS_C
#define UTILS_C

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
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>

#define lineSize 128  
#define msgSize 141   
#define msgNumber 200   
#define BUFFER_SIZE 2048
#define AWS_SIZE 1024
#define max(A,B) ((A) >= (B)?(A):(B))
#define STDIN 0
#define MAX_SIZE 1024

#define X "\x1b[38;5;196m x \x1b[0m"
#define PROMPT_TIMEOUT write(1, X, strlen(X))

#define M "\x1b[38;5;99m M \x1b[0m"
#define PING write(1, M, strlen(M))

#define C "\x1b[38;5;82m o \x1b[0m"
#define PROMPT_CONNECT write(1, C, strlen(C))

#define SYMBOL "\x1b[38;5;118m$ \x1b[0m"
#define PROMPT write(1, SYMBOL, strlen(SYMBOL))

extern int h_errno; 
extern int errno;

// Identificação do nosso servidor
struct msg_id {
	char name[50];
	struct in_addr addr;
	unsigned short term_port;
	unsigned short serv_port;
};

// Informação geral sobre a lista de servidores
struct servers_info
{
	struct servers_list *head;
	int n_servers;	// nº de servidores na lista
	int fd_link;	// fd utilizado para o accept
};

// informação sobre cada servidor da lista
struct servers_list
{
	int fd_tcp;		// fd que usamos para efetuar a partilha de informação
	struct servers_list * next;
	struct sockaddr_in addr; // guardamos o ip e o porto do servidor
	int flagMsg;		// para saber se a mensagem recebida foi partida porque excedeu o tamanho do buffer do read
	char auxstr[160];	// onde se guarda a parte incompleta da mensagem partida
};


// Informação geral sobre a lista de mensagens
struct msgs {
	struct msgList *head;	
	int curMsgNumber;	// nº de mensagens que já foram guardadas até ao momento (inclui as apagadas)
	int maxMsgNumber;	// nº de mensagens máximo que podemos guardar
	int LC;			// LC interno
};

// informação sobre cada mensagem da lista
struct msgList{
	int msgTime;	// clock logico da mensagem
	char msg[msgSize];	// a própria mensagem 
	struct msgList * next;
};

void read_args(int argc, char const *argv[], struct sockaddr_in *id_server, struct msg_id *msg_server, int *max_msg, int *reg_int);

void setDefaultSettings(struct sockaddr_in *id_server, socklen_t *addrlen, int *max_msg, int *reg_int);

void setup_sockets(int * fd, int *fdmax, int domain , int type);

void show_servers(struct sockaddr_in *id_server, socklen_t *addrlen);

void * mymalloc(int size);

void setSockaddrs(struct sockaddr_in *addr, unsigned long ip, unsigned short port, socklen_t *addrlen);


#endif