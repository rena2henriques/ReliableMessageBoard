#ifndef TCPSERVER_C
#define TCPSERVER_C

#include "utils.h"
#include "udpServer.h"


void get_servers(struct sockaddr_in *id_server, struct servers_info *myServer, struct msg_id *serverInfo);

void read_getservers(char buffer[BUFFER_SIZE], struct servers_info *myServer, struct msg_id *serverInfo);

void fill_servers_list(struct servers_info *myServer, char ip[16], char tcp_port[10], int fd);

void removeServer(struct servers_list **head, int fd);

int connect_2_server(struct servers_list **node, fd_set *rfds, int *fdmax );

void make_connections(struct servers_info *myServer, fd_set *rfds, int *fdmax);

void fdSET(struct servers_list *node, fd_set *rfds);

void fdISSET(struct servers_list *node, fd_set *rfds);

int sendMessages(struct msgs *msgStruct, int fd, struct servers_info *myServer, fd_set *rfds, int flag);

void receiveMsgList( struct msgs *msgStruct, char buffer[AWS_SIZE], struct servers_list * server);

void sendMsgRequest(struct servers_info *myServer, fd_set *rfds, int * flgReadState);

int attendTCPserver(struct servers_list * server, char buffer[AWS_SIZE], struct servers_info *myServer, fd_set *rfds, int *flgReadState);

void waitingTcpOperations(struct msgs *msgStruct, fd_set *rfds, struct servers_info *myServer, int *flgReadState);

void checkTcpOperation(int fd, char buffer[AWS_SIZE], struct servers_info *myServer, struct msgs *msgStruct, fd_set *rfds, struct servers_list * server);

void freeServersList(struct servers_info *myServer, fd_set *rfds);

void addMessages(char str[3], char buffer[AWS_SIZE], struct msgList *node, int flag);

void shareMsg(struct msgs *msgStruct, struct servers_info *myServer, fd_set *rfds);

void receiveIncompleteMsgList (struct msgs *msgStruct, char buffer[AWS_SIZE], struct servers_list * server);


#endif