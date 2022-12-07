#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "medicalso.h"

int main() {

	/*if(getenv("MAXCLIENTES") == NULL) {
			printf("MAXCLIENTES nao existe\n");
			return 0;
		}
		if(getenv("MAXMEDICOS") == NULL) {
			printf("MAXMEDICOS nao existe\n");
			return 0;
		}
		maxClientes = atoi(getenv("MAXCLIENTES"));
		maxMedicos = atoi(getenv("MAXMEDICOS"));*/

	//Select
	int nfd;
	fd_set read_fds;
	struct timeval tv;

	//Signal
	union sigval val;
	val.sival_int = 0;

	struct sigaction sa_alr;
	sa_alr.sa_sigaction = trataAlarm_medico;
	sigaction(SIGALRM, &sa_alr, NULL);

	cliente Cliente;
	medico Medico;

	if (mkfifo(BALCAO_FIFO, 0666) == -1) {
		if (errno == EEXIST) {
			printf("Ja existe um balcao em funcionamento!\n");
		}
	exit(-1);
	}
	printf("<CLINICA>\n");

	int b2c[2], c2b[2];
	char sd[100], resposta[100], mensagem[100], aux[100], pid_s[100], prioridade_char[2], vida[100];
	cliente utentes[MAXCLIENTES] = { };
	medico medicos[MAXMEDICOS] = { };
	int f, fdEnvio, nUtentes = 0, nEspecialistas = 0, i = 0, j = 0, pid_aux, inc = 0;
	int fdRec = open(BALCAO_FIFO, O_RDWR);
	pid_t pids_cli[MAXCLIENTES] = { };
	pid_t pids_med[MAXMEDICOS] = { };
	char confirmacao = 's';
	//cliente cli_ort[MAXCLIENTES] = { };

	char tipo;

	pipe(b2c);
	pipe(c2b);

	f = fork();
	if (f == -1)
		printf("Erro na criacao do fork");
	else if (f == 0) {

		close(b2c[1]);
		close(c2b[0]);

		close(STDIN_FILENO);
		dup(b2c[0]);
		close(b2c[0]);

		close(STDOUT_FILENO);
		dup(c2b[1]);
		close(c2b[1]);

		execl("classificador", "classificador", NULL);
		exit(-1);
	}
	else
	{
		close(b2c[0]);
		close(c2b[1]);
		do {
			tv.tv_sec = 5;
			tv.tv_usec = 0;
			FD_ZERO(&read_fds);
			FD_SET(0, &read_fds);
			FD_SET(fdRec, &read_fds);

			nfd = select(fdRec + 1, &read_fds, NULL, NULL, &tv);
			//printf("Resposta -> %d\n", nfd);


			if(FD_ISSET(0, &read_fds)){
				fflush(stdin);
				fgets(mensagem, 99, stdin);
				char temp[100];
				strcpy(temp, mensagem);
				for(i = 0; mensagem[i] != ' '; i++)
					aux[i] = mensagem[i];
				int as = i;
				aux[i] = '\0';
				if(strcmp(temp, "encerra\n") == 0){
					for(i = 0; i < nUtentes; i++){
						sigqueue(pids_cli[i], SIGINT, val);
					}
					for(i = 0; i < nEspecialistas; i++){
						sigqueue(pids_med[i], SIGINT, val);
					}
					encerrar();
				}else if(strcmp(temp, "utentes\n") == 0){
					printf("numero de utentes -> %d\n", nUtentes);
					if(nUtentes == 0)
						printf("Nao ha clientes\n");
					else{
						printf("Clientes em espera: \n");
						for(i = 0; i < nUtentes && utentes[i].estado == 0; i++)
							printf("-> %s[%d] - %s[%d]\n", utentes[i].nome, utentes[i].pid, utentes[i].especialidade, utentes[i].prioridade);

						printf("\nClientes em consulta: \n");
						for(i = 0; i < nUtentes && utentes[i].estado == 1; i++)
							printf("-> %s[%d] - %s[%d]\n", utentes[i].nome, utentes[i].pid, utentes[i].especialidade, utentes[i].prioridade);
					}
				}else if(strcmp(temp, "especialistas\n") == 0){
					printf("numero de especialistas -> %d\n", nEspecialistas);
					if(nEspecialistas == 0)
						printf("Nao ha especialistas ao servico\n");
					else{
						printf("Especialistas disponiveis: \n");
						for(i = 0; i < nEspecialistas && medicos[i].estado == 0; i++)
							printf("-> %s[%d] - %s\n", medicos[i].nome, medicos[i].pid, medicos[i].especialidade);

						printf("\nEspecialistas em consulta: \n");
						for(i = 0; i < nEspecialistas && medicos[i].estado == 1; i++)
							printf("-> %s[%d] - %s\n", medicos[i].nome, utentes[i].pid, medicos[i].especialidade);
					}
				}else if(strcmp(aux, "delut") == 0){
					for(i = 0, j = as+1; mensagem[j+1] != '\0'; i++, j++)
                        pid_s[i] = mensagem[j];
                    pid_s[i] = '\0';
					int pid = atoi(pid_s);
					for(i = 0; i < nUtentes; i++){
						if(utentes[i].pid == pid && utentes[i].estado == 0){
							val.sival_int = 1;
							sigqueue(utentes[i].pid, SIGINT, val);
                            printf("O cliente %s foi removido do sistema!\n", utentes[i].nome);
                            for(j = i; j < nUtentes; j++){                                      //Organizar o array
                                pids_cli[j] = pids_cli[j+1];
                                utentes[j] = utentes[j+1];
                            }
                            nUtentes--;
						}
					}
				}else if(strcmp(aux, "delesp") == 0){
					for(i = 0, j = as+1; mensagem[j+1] != '\0'; i++, j++)
                        pid_s[i] = mensagem[j];
                    pid_s[i] = '\0';
					int pid = atoi(pid_s);
					for(i = 0; i < nEspecialistas; i++){
						if(medicos[i].pid == pid && medicos[i].estado == 0){
							val.sival_int = 1;
							sigqueue(medicos[i].pid, SIGINT, val);
							printf("O medico %s foi removido do sistema!\n", medicos[i].nome);
                            for(j = i; j < nEspecialistas; j++){                                      //Organizar o array
                                pids_med[j] = pids_med[j+1];
                                medicos[j] = medicos[j+1];
                            }
                            nEspecialistas--;
						}
					}
				}
			}

			if(FD_ISSET(fdRec, &read_fds)){
				//fdRec = open(BALCAO_FIFO, O_RDWR); //Abrir o fifo
				read(fdRec, &tipo, sizeof(char));
				//printf("\nTipo: %c\n", tipo);
				if(tipo == 'c'){                                    //Caso seja o cliente a mandar algo
					read(fdRec, &Cliente, sizeof(Cliente));
					printf("\nSintomas do Utente n.%d (%s): %s",Cliente.pid, Cliente.nome, Cliente.sintomas);

					write(b2c[1], Cliente.sintomas, strlen(Cliente.sintomas));
					int size = read(c2b[0], resposta, 99);
					resposta[size] = '\0';
					printf("Especialidade e prioridade: %s\n", resposta);

					for(i = 0; resposta[i] != ' '; i++){
						Cliente.especialidade[i] = resposta[i];
					}
					*prioridade_char = resposta[i+1];
					Cliente.prioridade = atoi(prioridade_char);

					//strcpy(Cliente.resposta, resposta);
					utentes[nUtentes] = Cliente;
					pids_cli[nUtentes] = Cliente.pid;
					nUtentes++;

					sprintf(CLIENTE_FIFO_FINAL,CLIENTE_FIFO,Cliente.pid);
					fdEnvio = open(CLIENTE_FIFO_FINAL,O_RDWR);
					write(fdEnvio, &Cliente, sizeof(Cliente));
				}else if(tipo == 'm'){                                  //Caso seja o médico a mandar algo
					//int fdRec = open(BALCAO_FIFO, O_RDWR);
					read(fdRec, &Medico, sizeof(Medico));
					printf("O Dr.%s[%d] acabou de entrar ao servico na especialidade %s\n", Medico.nome, Medico.pid, Medico.especialidade);
					medicos[nEspecialistas] = Medico;
					pids_med[nEspecialistas] = Medico.pid;
					nEspecialistas++;
					//Confirmação do balcão de que está em funcionamento
					sprintf(MEDICO_FIFO_FINAL,MEDICO_FIFO, Medico.pid);
                    fdEnvio = open(MEDICO_FIFO_FINAL, O_RDWR);
                    write(fdEnvio, &confirmacao, sizeof(char));
                    close(fdEnvio);
				}else if(tipo == 'd'){                              //Caso seja uma desistência do cliente
					read(fdRec, &Cliente, sizeof(Cliente));
					for(j = 0; Cliente.pid != pids_cli[j]; j++);    //Procuramos o indice do cliente que quer desistir
					for(i = j; i < nUtentes; i++){                  //Organizar o array
						pids_cli[i] = pids_cli[i+1];
						utentes[i] = utentes[i+1];
					}
					nUtentes--;
					printf("O cliente %s saiu da fila de espera de %s.\n", Cliente.nome, Cliente.especialidade);
				}
//				else if (tipo == 'v') {
//					read(fdRec, &vida, sizeof(vida));
//					printf("\n%s\n", vida);
//				}
				else if (tipo == 's') {                            //Quando um medico quer sair
                    read(fdRec, &Medico, sizeof(Medico));
					for(j = 0; Medico.pid != pids_med[j]; j++);    //Procuramos o indice do medico que quer desistir
					for(i = j; i < nEspecialistas; i++){           //Organizar o array
						pids_med[i] = pids_med[i+1];
						medicos[i] = medicos[i+1];
					}
					nEspecialistas--;
					printf("O medico %s saiu do trabalho.\n", Medico.nome);
				}
				else if (tipo == 'y') {                             //Medico terminou a consulta
					read(fdRec, &Medico, sizeof(Medico));
                    for(i = 0; i < nEspecialistas; i++){
                        if(medicos[i].pid == Medico.pid){
                            medicos[i].estado = 0;
                        }
                    }
                }
			}

  			//CONSULTAS//
			for(i = 0; i < nUtentes; i++){
                for(j = 0; j < nEspecialistas; j++){
                    if(utentes[i].pid != 0 && utentes[i].estado == 0){
                        if(medicos[j].pid != 0 && medicos[j].estado == 0){
                            if(strcmp(medicos[j].especialidade, utentes[i].especialidade) == 0){
                                sprintf(MEDICO_FIFO_FINAL,MEDICO_FIFO, medicos[j].pid);
                                fdEnvio = open(MEDICO_FIFO_FINAL, O_RDWR);
                                tipo = 'b';
                                write(fdEnvio, &tipo, sizeof(char));
                                write(fdEnvio, &utentes[i], sizeof(Cliente));
                                close(fdEnvio);

                                sprintf(CLIENTE_FIFO_FINAL,CLIENTE_FIFO, utentes[i].pid);
                                fdEnvio = open(CLIENTE_FIFO_FINAL, O_RDWR);
                                write(fdEnvio, &medicos[j], sizeof(Medico));
                                close(fdEnvio);

                                utentes[i].estado = 1;
                                medicos[j].estado = 1;
                            }
                        }
                    }
				}
			}
		}while (1);

		close(fdRec);
		close (fdEnvio);
		write(b2c[1], "#fim\n", 5); //encerra o classificador
		wait(&f);
		close(b2c[1]);
		close(c2b[0]);
	}

	unlink(BALCAO_FIFO);
}

