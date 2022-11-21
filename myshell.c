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
        pid = fork();
        if (pid<0){
            fprintf(stderr,"Error a la hora de hacer el fork\n");        
        }else if(pid==0){
            execvp(comando->commands[0].argv[0], comando->commands->argv);
            fprintf(stderr,"Error a la hora de hacer el comando\n");
            exit(1);
        }else{
            wait(NULL);
        }
        printf("\nmsh> ");
        
    }
    return 0;
}
