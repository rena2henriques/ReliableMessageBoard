#include "user_cmds.h"
 
int readCmds(struct Flags * flags, struct opt2_info * opt2info, struct serverStruct * ServStruct, int fdID){
	/**************************
	*		user input
	**************************/
	char input[BUFFER_SIZE];
	//Limpar o buffer que receberá o input do utilizador
	memset(input, '\0', BUFFER_SIZE);
	// Variavel que representa o numero de argumentos que o utilizador colocou
	int argNumber = -1;

	// É suposto serem entre 1 e 2 argumentos
	//opt1 para o primeiro
	//opt2 para o segundo, caso haja
	char opt1[20], opt2[msgSize];
	memset(opt1, '\0', 20);
	//importante limpar o segundo
	memset(opt2, '\0', msgSize);


	// lê a linha inteira que foi escrita no terminal
	if (fgets(input, BUFFER_SIZE , stdin) == NULL){
		printf("erro no fgets ao ler do terminal\n");
		// É mostrada a prompt
		PROMPT;
	}

	// vai buscar as duas strings ao terminal
	// caso nao consiga ler bem nenhuma existe um erro
	if((argNumber = sscanf(input,"%20s %140[^\n]s", opt1, opt2)) == 0 ){
		printf("erro na separação do sscanf\n");
		PROMPT;
		return 0;
	}
	

	// se exit for escrito, o programa acaba
	if(strcmp(opt1, "exit") == 0){

		// dar free de tudo
		close(ServStruct->fd);
		freeBlocked(&(ServStruct->head));
		freeOptList(opt2info, 0);
		close(fdID);
		return -1;
	}
	/// show_servers ou exit
	else if(strcmp(opt1, "show_servers") == 0 && argNumber == 1){
		//getServers
		flags->showServers++;
		insertOpt("show_servers", SHOW_SERVERS, opt2info);
	}
	else if(strcmp(opt1, "publish") == 0 && argNumber == 2){
			
			// É incrementado o numero de publishs por tratar
			flags->publish++;
			// É adicionada essa tarefa à lista das Operções
			insertOpt(opt2, PUBLISH, opt2info);
	}

	else if(strcmp(opt1, "show_latest_messages") == 0 && argNumber == 2 && atoi(opt2) > 0){
			// É incrementado o numero de show_latest_messages por tratar
			flags->showLatMsg++;
			// É adicionada essa tarefa à lista das Operções
			insertOpt(opt2, SHOW_LATEST_MESSAGES, opt2info);

	}

	else{
		printf("Invalid input, try:\n");
		printf("\tshow_servers\n\tpublish [n]\n\tshow_latest_messages\n\texit\n\n");
		// É mostrada a promp
		PROMPT;
		return 0;
	}

	return 1;
}