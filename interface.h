#ifndef INTERFACE_C
#define INTERFACE_C

#include "udpServer.h"
#include "tcpServer.h"
#include "usercmds.h"


void interface(struct sockaddr_in *id_server, struct msg_id * msgS , int interval, int max_msg); 

#endif