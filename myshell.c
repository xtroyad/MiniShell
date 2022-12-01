#include <stdio.h>
#include "parser.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
int pid;

void manejador(int sig){
	if(pid == 0){
		kill(getpid(), 9);
	}
}

int main(){ 
    //VARIABLES DEL PROGRAMA
    char buf[1024];
    tline * comando;
    FILE * pf;
    printf("msh> ");

    signal (SIGINT, manejador);//Ignoramos la señal CTLR+C


    while(fgets(buf, 1024, stdin)){
        
        comando = tokenize(buf); 
        
        if (comando->ncommands==0) {
            printf("\nmsh> ");
			continue;
		}

        //EXIT
        if (strcmp(comando->commands[0].argv[0],"exit")==0) {//cuando tenemos exit or CTLR+D terminamos
            kill(getpid(),9); //mandamos un SIGKILL para MyShell
        }

        //CD
        if (strcmp(comando->commands[0].argv[0],"cd")==0) {//cuando tenemos exit or CTLR+D terminamos
            char *dir; 
            char buffer[512];

            //si solo tiene el nombre de cd ...
            if (comando->commands[0].argc == 1){
                dir = getenv("HOME");
                if(dir == NULL){
                    fprintf(stderr, "No existe la variable $HOME\n");
                }
            }

            //si tiene el nombre del cd y un argumento
            if (comando->commands[0].argc == 2){
                dir = comado->commands[0].argv[1];
            }

            //si tiene el nombre y mas de un argumento 
            if (comando->commands[0].argc > 2){
               fprintf(stderr, "Numero de argumentos incorrecto para el cd\n");

            }

            //si salio mal el cambio de directorio
            if(chdir(dir) != 0){
                fprintf(stderr, "Error al cambiar de directorio\n");
            }
        }

        pid = fork();
        if (pid<0){
            fprintf(stderr,"Error a la hora de hacer el fork\n");        
        }else if(pid==0){
             //-----------------------------------------------------------------------------------------
            if (comando->redirect_input != NULL) {
                fclose(stdin);
                pf=fopen(comando->redirect_input,"r");
		    }
            //-----------------------------------------------------------------------------------------
            if (comando->redirect_output != NULL) {
                fclose(stdout);
                pf=fopen(comando->redirect_output,"w");
            }
            //-----------------------------------------------------------------------------------------
            if (comando->redirect_error != NULL) {
                fclose(stderr);
                pf=fopen(comando->redirect_error,"w");
		    }
            //-----------------------------------------------------------------------------------------
            if (pf==NULL){
                fprintf(stderr,"Error a la hora de abrir el fichero ");
            }
            execvp(comando->commands[0].argv[0], comando-> commands[0].argv);
            fprintf(stderr,"Error a la hora de hacer el comando\n");

            exit(1);
        }else{
            wait(NULL);
        }
        printf("\nmsh> ");
        
    }
    return 0;
}
