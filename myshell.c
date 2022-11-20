#include <stdio.h>
#include "parser.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
int main(){ 
    char buf[1024];
    tline * comando;
    signal (SIGINT, manejador);//Ignoramos la señal CTLR+C
    int pid;

    
    printf("msh> ");
    while(fgets(buf, 1024, stdin)){
        
        comando = tokenize(buf); 
        if (strcmp(comando->commands[0].argv[0],"exit")==0) {//cuando tenemos exit or CTLR+D terminamos
            kill(getpid(),19); //mandamos un SIGSTOP para MyShell
            exit(1);
        }
        pid = fork();
        if (pid<0){
            fprintf(stderr,"Error a la hora de hacer el fork");        
        }else if(pid==0){
            execvp(comando->commands[0].argv[0], comando->commands->argv);
            fprintf(stderr,"Error a la hora de hacer el comando");
            exit(1);
        }else{
            wait(NULL);
        }
        printf("msh> ");
        
    }
    return 0;
}

int manejador(int sig){
    if (getpid()!=0){
        exit(0);
    }else{

    }
}
