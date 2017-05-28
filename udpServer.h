#ifndef UDPSERVER_C
#define UDPSERVER_C

#include "utils.h"
#include "tcpServer.h"


void join(struct sockaddr_in *id_server, struct msg_id *msg_server, int fd);

void receiveMsg(struct msg_id * msgS, int fd_client);

void bufferAnalyser(char buffer[BUFFER_SIZE], struct msgs * msgStruct, int fd, struct sockaddr *addr, struct servers_info *myServer, fd_set *rfds);

int insertMsgList(char data[msgSize], struct msgList ** head, struct msgs * msgStruct, int LC);

void getMessages(char ** answer, struct msgs * msgStruct, int n);

void reply(char * answer, int fd, struct sockaddr *addr);

int publishMsg(char data[], struct msgList ** head, struct msgs * msgStruct, int list_max_size);

void remove_list_tail(struct msgList ** head);

void freeMsgList(struct msgs *msgStruct);

void fillWithMessages(char *buffer, struct msgList *node, int n, int *counter);

void joinTest(struct sockaddr_in *id_server, struct msg_id *msg_server, int fd, char id[100], int *flagJoinTest);

#endif