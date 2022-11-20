#include <stdio.h>
#include "parser.h"
#include <signal.h>

int main(){ 
    char buf[1024];
    tline * comando;
    signal (SIGINT, SIG_IGN);//Ignoramos la señal CTLR+C
    int pid;

    while(1){
        printf("msh> ");

        while(fgets(buf, 1024, stdin)){
            comando = tokenize(buf); 
            if (comando->commands->argv=="exit"){//cuando tenemos exit or CTLR+D terminamos
                kill(getpid(),19); //mandamos un SIGSTOP para MyShell
            }
            pid = fork();
            if (pid<0){
                fprintf(stderr,"Error a la hora de hacer el fork");        
            }else if(pid>0){
                execvp(comando->commands->argv[1], comando->commands->argv+1);
                fprintf(stderr,"Error a la hora de hacer el comando");
                exit(1);
            }else{
                wait(NULL);
            }
            
        }

    }


}

