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
    char buf[1024];
    tline * comando;
    int pp[2];
    FILE * fd;
    char buf[1024];
    signal (SIGINT, manejador);//Ignoramos la señal CTLR+C
    
    printf("msh> ");
    while(fgets(buf, 1024, stdin)){
        
        comando = tokenize(buf); 
        if (comando->ncommands==0) {
            printf("\nmsh> ");
			continue;
		}
        if (strcmp(comando->commands[0].argv[0],"exit")==0) {//cuando tenemos exit or CTLR+D terminamos
            kill(getpid(),9); //mandamos un SIGSTOP para MyShell
        }
        pipes(pp);
        pid = fork();
        if (pid<0){
            fprintf(stderr,"Error a la hora de hacer el fork\n");        
        }else if(pid==0){
            execvp(comando->commands[0].argv[0], comando->commands->argv);
            fprintf(stderr,"Error a la hora de hacer el comando\n");
            exit(1);
        }else{
            // //-----------------------------------------------------------------------------------------
            
            // if (comando->redirect_input != NULL) {
            //     printf("redirección de entrada: %s\n", comando->redirect_input);
            //     close(pp[1]);
            //     fd=fdopen(pp[0],"r");
            // }
            // //-----------------------------------------------------------------------------------------
            
            wait(NULL);
        }
        
        printf("\nmsh> ");
        
    }
    return 0;
}
