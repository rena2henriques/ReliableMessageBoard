#ifndef USERCMDS_C
#define USERCMDS_C

#include "utils.h"

void showServers(struct servers_info myServer);

void showMessages(struct msgList *head);

void printMessage(struct msgList *node, int LC);

#endif