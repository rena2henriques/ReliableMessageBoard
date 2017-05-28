CFLAGS=-g -Wall -std=gnu11

default: all
	
all: rmb msgserv

msgserv: msgserv.c udpServer.c utils.c interface.c tcpServer.c usercmds.c
	gcc msgserv.c udpServer.c utils.c interface.c tcpServer.c usercmds.c -o msgserv $(CFLAGS)

rmb: main.c user_cmds.c readArgs.c handleOpt.c utilsRmb.c
	gcc main.c user_cmds.c readArgs.c handleOpt.c utilsRmb.c -o rmb $(CFLAGS)

clean:
	rm -f *.o rmb msgserv