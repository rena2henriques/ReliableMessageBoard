/*****************************************************************************/
/*               Projeto de RCI - Reliable Message Board                     */
/*                   Manuel Reis & Renato Henriques                          */
/*****************************************************************************/

#include "interface.h"

int main(int argc, char const *argv[])
{
	/* id_server é o servidor de identidades */
	struct sockaddr_in id_server;

	socklen_t addrlen; // tamanho do servidor de identidades

	struct msg_id msg_server; // onde guardamos os dados sobre o nosso servidor

	int max_msg = -1; //nº máx de mensagens guardadas num servidos
	int reg_int = -1; //intervalo entre registos
	
	// gerar seeds random
	struct timeval t1;
	gettimeofday(&t1, NULL);
	srand(t1.tv_usec * t1.tv_sec); // para ter uma diferente seed a cada inicio do programa

	void (*old_handler)(int);

	// utilizado para o programa ignorar erros do tipo SIGPIPE
	if( (old_handler = signal(SIGPIPE, SIG_IGN)) == SIG_ERR) {
		printf("Houve um conflito a ignorar os sinais SIGPIPEs\n");
		exit(1);
	}

	// predefine o valor do ip e do porto do servidor de identidades com os valores default 
	setDefaultSettings( (struct sockaddr_in *) &id_server, &addrlen, &max_msg, &reg_int);

	// lê os argumentos que são recebidos no inicio do programa
	read_args(argc, argv, &id_server, &msg_server, &max_msg, &reg_int);

	// corre a interface de utilizador
	interface(&id_server, (struct msg_id *)&msg_server, reg_int, max_msg);
	
	
	return 0;
}