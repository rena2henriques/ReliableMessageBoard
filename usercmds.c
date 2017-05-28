#include "usercmds.h"

// imprime os servidores com que o servidor tem uma conexão TCP
void showServers(struct servers_info myServer) {

	// à partida se um servidor está na lista significa que a comunicação já foi feita <----- confirmar este pensamento

	while(myServer.head != NULL) {

		printf("%s || %hu\n", inet_ntoa(myServer.head->addr.sin_addr), ntohs(myServer.head->addr.sin_port));

		myServer.head = myServer.head->next;
	}

	return;
}

// imprime todas as mensagens guardadas no servidor
void showMessages(struct msgList *head) {

	// as mensagens na lista já devem estar ordenadas pelo seu tempo lógico
	int LC = 1;
	printMessage(head, LC);
	printf("\n");

	return;
}

// imprime recursivamente da mais antiga para a mais recente
void printMessage(struct msgList *node, int LC) {

	if(node == NULL){
		return;
	}

	printMessage(node->next, LC);

	if(LC == 1){
		printf("%d;", node->msgTime);
	}

	printf("%s\n", node->msg);

	return;
}