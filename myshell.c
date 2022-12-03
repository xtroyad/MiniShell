#include <stdio.h>
#include "parser.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>




typedef struct {
    int p[2];
} tPipe;

//elemento de la lista de lineas al bg
typedef struct {
    int *pidsLineEst;
    int numPids; //tamaño lista de pids
    int contTerminados;
    char *linea; 
} tBgElem;

char buf[1024];

tline * comando;
int pid;
tPipe *pipes; //array de pipes 
int i;

int status; 
int tamañoBG; 

//lista de pids de los mandatos de la linea
pid_t *pidsLine; //aqui guardar pids de la linea y si hay que mandar a bg copiar en la de la estructura 


//lista de prodesos en bg 
tBgElem *procBG;

void manejador(int sig){
	if(pid > 0){
		kill(pid, 9);
	}
}

int main(){ 
    tamañoBG = 0;

    procBG = calloc(3, sizeof(tBgElem)); // lista de elementos de lineas al bg

    signal (SIGINT, manejador);//Ignoramos la señal CTLR+C

    printf("msh> ");
    while(fgets(buf, 1024, stdin)){
        pid=0;
        comando = tokenize(buf); 

        //comprobar hijos bg

        comprobacionZombies();
  
        if (comando->ncommands==0) {
            printf("\nmsh> ");
			continue;
		}
        if (strcmp(comando->commands[0].argv[0],"exit")==0) {//cuando tenemos exit or CTLR+D terminamos
            kill(getpid(),9); //mandamos un SIGKILL para MyShell
        }
        if (strcmp(comando->commands[0].argv[0],"cd")==0) {
            cd();
            continue;
        }

        pipes = (tPipe *)calloc(comando->ncommands-1,sizeof(tPipe));

//------------------------------------------------------------------------------------------------------------------------------------------
        pidsLine = (int *)calloc(comando->ncommands, sizeof(int)); //lista de los pids de los mandatos de la linea


//------------------------------------------------------------------------------------------------------------------------------------------


        if (comando->ncommands==1){
            pid=fork();
            pidsLine[i] = pid;
            i=0;
        }else{
            nmandatos();
        }
    
        if (pid==0){

            redirec();
                
            execvp(comando->commands[i].argv[0], comando-> commands[i].argv);
            fprintf(stderr,"Error a la hora de hacer el comando\n");

            exit(1);

        }
        else{
            if(comando->background==1){
                tamañoBG++;
                procBG = (tBgElem *)realloc(procBG, tamañoBG*sizeof(tBgElem));

                procBG[tamañoBG-1].pidsLineEst = pidsLine; //Guarda pids de la linea en la lista del elem 
                procBG[tamañoBG-1].numPids = comando->ncommands; //guarda el num de mandatos de la linea en el numpids
                procBG[tamañoBG-1].contTerminados = 0; //inicia contador a cero 
                pidsLine = NULL; 
            }
            else {
                for(int j =0; j<comando->ncommands;j++){
                    wait(NULL);
                }

            }
            //for(int k = 0; k<comando->ncommands; k++){
                //printf("%d", pidsLine[k]);
            //}
           
        }

        printf("\nmsh> ");
    }
    return 0;
}


//-----------------------------------------------------------------------------------------------------------------------------------------
void cd(){
    
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
        dir = comando->commands[0].argv[1];
    }

    //si tiene el nombre y mas de un argumento 
    if (comando->commands[0].argc > 2){
    fprintf(stderr, "Numero de argumentos incorrecto para el cd\n");

    }

    //si salio mal el cambio de directorio
    if(chdir(dir) != 0){
        fprintf(stderr, "Error al cambiar de directorio\n");
    }
    printf("\nmsh> ");
        
    
}


void nmandatos(){
    for(i=0; i<comando->ncommands;i++){
    
        if (i==0){
        
            pipe(pipes[i].p);//pipe(pp1);
            pid=fork();
            
            if (pid==0){ //Somos el hijo
                
                close(pipes[i].p[0]); //close(pp1[0]);
                dup2(pipes[i].p[1], STDOUT_FILENO); //dup2(pp1 [1], STDOUT_FILENO); 
                close(pipes[i].p[1]);//close(pp1[1]);
                break;
            }else{ //Somos el padre
                pidsLine[i] = pid; //guarda pid en la lista 
                close(pipes[i].p[1]);//close(pp1[1]);
                
            
            }
        }else if(i<comando->ncommands-1){

            pipe(pipes[i].p);//pipe(pp2);		  /* comunica grep y wc */
            pid = fork();
            
            if (pid==0){ //Somos el hijo
                
                close(pipes[i].p[0]);//close(pp2[0]);

                dup2(pipes[i-1].p[0], STDIN_FILENO); //dup2(pp1[0], STDIN_FILENO); 
                close(pipes[i-1].p[0]);//close(pp1[0]);

                dup2(pipes[i].p[1], STDOUT_FILENO);//dup2(pp2[1], STDOUT_FILENO);
                close(pipes[i].p[1]);//close(pp2[1]);
                break;

            }else{ //Somos el padre
                pidsLine[i] = pid;
                close(pipes[i-1].p[0]);//close(pp1[0]);
                close(pipes[i].p[1]);//close(pp2[1]);
            
            }

        }else{
            pid = fork();
            pidsLine[i] = pid;
            if (pid==0){
            
                dup2(pipes[i-1].p[0],STDIN_FILENO);
                close(pipes[i-1].p[0]);
                break;
            }
        }

        
    }
}

void redirec(){
    if( (comando->ncommands==1) ||(i==0 || i==comando->ncommands-1 )){


        if (comando->redirect_input != NULL && i==0) {
            
            int fd=open(comando->redirect_input,  O_RDONLY);
            if (fd <0){
                fprintf(stderr, "Error al abrir el fichero");
                exit(1);
            }
            dup2(fd, 0);

        }
        //-----------------------------------------------------------------------------------------
        if (comando->redirect_output != NULL && i==comando->ncommands-1 ) {

            int fd=open(comando->redirect_output,  O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR| S_IWUSR | S_IRGRP|S_IROTH );
            if (fd <0){
                fprintf(stderr, "Error al abrir el fichero");
                exit(1);
            }
            dup2(fd, 1);
            
        }
        //-----------------------------------------------------------------------------------------
        if (comando->redirect_error != NULL && i==comando->ncommands-1) {
            int fd=open(comando->redirect_error,  O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR| S_IWUSR | S_IRGRP|S_IROTH );
            if (fd <0){
                fprintf(stderr, "Error al abrir el fichero");
                exit(1);
            }
            dup2(fd, 2);
        }
        //-----------------------------------------------------------------------------------------

    }
}

void comprobacionZombies(){
    for(int i = 0; i < tamañoBG; i++){ //recorre lista procBG
        for(int j = 0; j < procBG[i].numPids; j++){ //recorre lista del elem de procBG
            waitpid(procBG[i].pidsLineEst[j], &status, WNOHANG);

            if (WIFEXITED(status) == 0){ //si termina de manera normal...
                procBG[i].contTerminados++;
            }

            //si han terminado todos los procesos de esa linea ponemos la lista a NULL
            if(procBG[i].contTerminados == procBG[i].numPids){ 
                //procBG[i].pidsLineEst[j] = NULL; 
                reorganizar(i);
            }
        }
    }
}

void reorganizar(int n){
    for(int j=n; n<tamañoBG-1; j++){
        procBG[j]=procBG[j+1];
    }
    procBG=(tBgElem*)realloc(procBG,--tamañoBG);
}

