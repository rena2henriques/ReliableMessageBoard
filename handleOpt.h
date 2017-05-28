#ifndef HANDLE_OPT_C
#define HANDLE_OPT_C

#include "utilsRmb.h"


void getServers(int fdID, struct sockaddr_in *id_server, socklen_t * addrlenID, struct serverStruct * ServStruct, char buffer[BUFFER_SIZE], int *fdmax, struct Flags *flags, struct opt2_info *opt2info, char **bufferMessages);

void handleOpt(struct Flags *flags, struct serverStruct *ServStruct, struct opt2_info *opt2info, char buffer[BUFFER_SIZE], char publishBuffer[180], char showLatMsgBuffer[50], int fdID, socklen_t *addrlenID, struct sockaddr_in *id_server, char **bufferMessages);

int counterExceeded( struct Flags *flags, struct serverStruct * ServStruct, int fdID, socklen_t *addrlenID, struct sockaddr_in *id_server, struct opt2_info * opt2info);


#endif