#include "medicalso.h"

int main(int argc, char* argv[]){
	if(argc != 2){
		printf("Por favor indique o seu nome!\n");
		exit(1);
	}

	//Signal
	struct sigaction sa_int;
	struct sigaction sa_alr;
	sa_int.sa_sigaction = trataSinal_cliente;
	sa_int.sa_flags = SA_SIGINFO;
	sigaction(SIGINT, &sa_int, NULL);
	sa_alr.sa_sigaction = trataAlarm_cliente;
	sigaction(SIGALRM, &sa_alr, NULL);

	//Select
	int nfd;
	fd_set read_fds;
	struct timeval tv;

	int i;
	char c, tipo;

	cliente Cliente;
	medico Medico;

	strcpy(Cliente.nome, argv[1]);

	Cliente.pid = getpid();
	Cliente.tipo = 'c';
	Cliente.estado = 0;
	printf("\nBem vindo! O seu PID e: %d\n", Cliente.pid);
	sprintf(CLIENTE_FIFO_FINAL, CLIENTE_FIFO, getpid());

	if(mkfifo(CLIENTE_FIFO_FINAL, 0666) == -1){
		if(errno == EEXIST)
			printf("FIFO ja existe\n");
	exit(-1);
	}

	char conversa[100], resposta[100];
	pid_t pid_med;


	printf("Quais sao os seus sintomas?: ");
	fgets(Cliente.sintomas, 99, stdin);

	int fdEnvio = open(BALCAO_FIFO, O_RDWR);
	write(fdEnvio, &Cliente.tipo, sizeof(char));
	write(fdEnvio, &Cliente, sizeof(Cliente));

	alarm(5);   //Espera por uma confirmação do balcão durante 5 segundos

	int fdRec = open(CLIENTE_FIFO_FINAL, O_RDWR);
	read(fdRec, &Cliente, sizeof(Cliente));

	alarm(0);

	printf("Relatorio: \n	Especialidade -> %s\n	Prioridade -> %d\n", Cliente.especialidade, Cliente.prioridade);

	printf("\nJa se encontra em fila de espera... Por favor aguarde!\n");
	printf("\nSe quiser desistir pressione 'd'\n");
	do{
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		FD_ZERO(&read_fds);
		FD_SET(0, &read_fds);
		FD_SET(fdRec, &read_fds);

		nfd = select(fdRec + 1, &read_fds, NULL, NULL, &tv);
		//printf("Resposta -> %d\n", nfd);

		if(FD_ISSET(0, &read_fds)){

			scanf("%c", &c);
			if(c != 'd')
				printf("Por favor aguarde ou pressione 'd' para desistir...\n");
			else{
				int fdEnvio = open(BALCAO_FIFO, O_RDWR);
				Cliente.tipo = 'd';
				write(fdEnvio, &Cliente.tipo, sizeof(char));
				write(fdEnvio, &Cliente, sizeof(Cliente));
				printf("Desistiu de estar a espera!\n");
				encerrar_cliente();
			}
		}

		if(FD_ISSET(fdRec, &read_fds)){
			read(fdRec, &Medico, sizeof(Medico));
			printf("\nVai ser atendido pelo Dr.%s[%d].\n", Medico.nome, Medico.pid);
			break;
		}
	}while(1);

	sprintf(MEDICO_FIFO_FINAL,MEDICO_FIFO, Medico.pid);
	//fdEnvio = open(MEDICO_FIFO_FINAL, O_RDWR);

	close(fdRec);
	close(fdEnvio);

	printf("\nConsulta com o Dr.%s iniciada!\n", Medico.nome);

	fdEnvio = open(MEDICO_FIFO_FINAL, O_RDWR);
	fdRec = open(CLIENTE_FIFO_FINAL, O_RDWR);

	do{
        tv.tv_sec = 5;
		tv.tv_usec = 0;
		FD_ZERO(&read_fds);
		FD_SET(fdRec, &read_fds);

		nfd = select(fdRec + 1, &read_fds, NULL, NULL, &tv);
		//printf("Resposta -> %d\n", nfd);

        if(FD_ISSET(fdRec, &read_fds)){
            //fdRec = open(CLIENTE_FIFO_FINAL, O_RDONLY);
            int size = read(fdRec, &resposta, sizeof(resposta));
            //close(fdRec);
            printf("\n[MED]-> %s", resposta);
            if(strcmp(resposta, "adeus\n") == 0)
                break;
            printf("\n[CLI]-> ");
            fgets(conversa, 99, stdin);
            //strcat(conversa, '\0');
            //fdEnvio = open(MEDICO_FIFO_FINAL, O_WRONLY);
            tipo = 'c';
            write(fdEnvio, &tipo, sizeof(char));
            write(fdEnvio, &conversa, sizeof(conversa));
            //close(fdEnvio);
            if(strcmp(conversa, "adeus\n") == 0)
                break;
        }
	}while(1);

	close(fdRec);
	close(fdEnvio);

	unlink(CLIENTE_FIFO_FINAL);

}
