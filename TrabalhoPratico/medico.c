#include "medicalso.h"

int main(int argc, char* argv[]){
	if(argc != 3){
		printf("Por favor indique o seu nome e a sua especialidade, respetivamente!\n");
		exit(1);
	}

	//Select
	int nfd;
	fd_set read_fds;
	struct timeval tv;

	//Signal
	struct sigaction sa_int;
	struct sigaction sa_alr;
	sa_int.sa_sigaction = trataSinal_medico;
	sa_int.sa_flags = SA_SIGINFO;
	sigaction(SIGINT, &sa_int, NULL);
	sa_alr.sa_sigaction = trataAlarm_medico;
	sigaction(SIGALRM, &sa_alr, NULL);

	union sigval val;
	val.sival_int = 0;

	medico Medico;
	cliente Cliente;

	strcpy(Medico.nome, argv[1]);
	strcpy(Medico.especialidade, argv[2]);

	pid_t pid_cliente;
	char conversa[100], resposta[100], info[100];
	char confirm, tipo;

	Medico.tipo = 'm';
	Medico.estado = 0;
	Medico.pid = getpid();
	sprintf(MEDICO_FIFO_FINAL, MEDICO_FIFO, getpid());

	if(mkfifo(MEDICO_FIFO_FINAL, 0666) == -1){
		if(errno == EEXIST)
			printf("FIFO ja existe\n");
	}

	int fdRec;
	int fdEnvio = open(BALCAO_FIFO, O_RDWR);
	write(fdEnvio, &Medico.tipo, sizeof(char));
	write(fdEnvio, &Medico, sizeof(Medico));

//	alarm(5); //Espera por uma confirmação do balcão durante 5 segundos
//
//	read(fdRec, &confirm, sizeof(char));
//
//	alarm(0);

	//alarm(20);

	do{
        printf("Aguarde por um utente...\n");

        close(fdEnvio);

        fdRec = open(MEDICO_FIFO_FINAL, O_RDWR);

        do{
            tv.tv_sec = 5;
            tv.tv_usec = 0;
            FD_ZERO(&read_fds);
            FD_SET(0, &read_fds);
            FD_SET(fdRec, &read_fds);

            nfd = select(fdRec + 1, &read_fds, NULL, NULL, &tv);
            //printf("Resposta -> %d\n", nfd);

            if(FD_ISSET(0, &read_fds)){

                fgets(info, 99, stdin);
                if(strcmp(info, "sair\n") != 0)
                    printf("Por favor aguarde ou escreva 'sair' para sair...\n");
                else{                                                                    //Medico vai sair do trabalho
                    int fdEnvio = open(BALCAO_FIFO, O_RDWR);
                    Medico.tipo = 's';
                    write(fdEnvio, &Medico.tipo, sizeof(char));
                    write(fdEnvio, &Medico, sizeof(Medico));
                    printf("Ja saiu do trabalho!\n");
                    encerrar_medico();
                }
            }

            if(FD_ISSET(fdRec, &read_fds)){
                read(fdRec, &tipo, sizeof(char));
                if(tipo == 'b'){
                    read(fdRec, &Cliente, sizeof(Cliente));

                    printf("Vai receber o utente %s[%d].\n", Cliente.nome, Cliente.pid);
                    sprintf(CLIENTE_FIFO_FINAL,CLIENTE_FIFO,Cliente.pid);
                        /*fdEnvio = open(CLIENTE_FIFO_FINAL, O_RDWR);
                        write(fdEnvio, &Medico, sizeof(Medico));
                        close(fdEnvio);*/
                    close(fdRec);

                    fdEnvio = open(CLIENTE_FIFO_FINAL, O_RDWR);
                    fdRec = open(MEDICO_FIFO_FINAL, O_RDWR);

                    printf("\n\n%d\n\n", Cliente.pid);

                    printf("\n[MED]-> ");
                    fgets(conversa, 99, stdin);
                    //strcat(conversa, '\0');
                    //fdEnvio = open(CLIENTE_FIFO_FINAL, O_WRONLY);
                    write(fdEnvio, &conversa, sizeof(conversa));
                    //close(fdEnvio);
                    if(strcmp(conversa, "adeus\n") == 0)
                        break;
                }else if(tipo == 'c')
                    read(fdRec, &resposta, sizeof(resposta));
                    printf("\n[CLI]-> %s", resposta);
                    printf("\n[MED]-> ");
                    fgets(conversa, 99, stdin);
                    strcat(conversa, '\0');
                    //fdEnvio = open(CLIENTE_FIFO_FINAL, O_WRONLY);
                    write(fdEnvio, &conversa, sizeof(conversa));
                    //close(fdEnvio);
                    if(strcmp(conversa, "adeus\n") == 0)
                        break;
                }

        }while(1);

        fdEnvio = open(BALCAO_FIFO, O_RDWR);
        tipo = 'y';
        write(fdEnvio, &tipo, sizeof(char));
        write(fdEnvio, &Medico, sizeof(Medico));
	}while(1);

	close(fdRec);
	close(fdEnvio);

	unlink(MEDICO_FIFO_FINAL);
}

