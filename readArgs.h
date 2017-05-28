#ifndef READ_ARGS_C
#define READ_ARGS_C

#include "utilsRmb.h"

void setDefaultSettings(struct sockaddr_in *id_server, socklen_t *addrlen);

void read_args(int argc, char const *argv[], struct sockaddr_in *id_server);

#endif
