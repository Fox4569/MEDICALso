#ifndef HEADER_FILE
#define HEADER_FILE

#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <pthread.h>
#include <time.h>


#define BALCAO_FIFO "BalcaoFifo"
#define MEDICO_FIFO "Medico%d"
#define CLIENTE_FIFO "Cliente%d"
#define MAXCLIENTES 10
#define MAXMEDICOS 10

char MEDICO_FIFO_FINAL[100];
char CLIENTE_FIFO_FINAL[100];

int nUtentes = 0;
int nEspecialistas = 0;
int tempo = 30;

typedef struct{
	char nome[10];
	char sintomas[100];
	char resposta[100];
	char especialidade[15];
	int prioridade;
	char tipo;
	int estado;
	pid_t pid;
} cliente;

typedef struct{
	char nome[10];
	char especialidade[15];
	char tipo;
	int estado;
	pid_t pid;
} medico;

void encerrar(){
	unlink(BALCAO_FIFO);
	exit(1);
}

void encerrar_cliente(){
	unlink(CLIENTE_FIFO_FINAL);
	exit(1);
}

void encerrar_medico(){
    unlink(MEDICO_FIFO_FINAL);
    exit(1);
}


void trataSinal_cliente(int signum, siginfo_t *info, void *secret){
	if(info->si_value.sival_int == 1){
		printf("Foi tirado do sistema pelo administrador!\n");
	}else{
		printf("\nO balcao vai encerrar!\n");
	}
	unlink(CLIENTE_FIFO_FINAL);
	exit(1);
}

void trataSinal_medico(int signum, siginfo_t *info, void *secret){
	if(info->si_value.sival_int == 1){
		printf("Foi tirado do sistema pelo administrador!\n");
	}else{
		printf("\nO balcao vai encerrar!\n");
	}
	unlink(MEDICO_FIFO_FINAL);
	exit(1);
}


void trataAlarm_cliente(int signum, siginfo_t *info, void *secret){
	printf("\nProvavelmente o balcao nao esta em funcionamento!\n");
	unlink(CLIENTE_FIFO_FINAL);
	exit(1);
}

void trataAlarm_medico(int signum, siginfo_t *info, void *secret){
        char lindo[] = "\nLindo\n";
        char tipo = 'v';
        int fdEnvio = open(BALCAO_FIFO, O_RDWR);
        write(fdEnvio, &tipo, sizeof(char));
        write(fdEnvio, &lindo, sizeof(lindo));
        close(fdEnvio);
}


#endif
