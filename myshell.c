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

//-------------------------------------------------------------------------------------------------------------
//DECLARACION DE VARIABLES
//-------------------------------------------------------------------------------------------------------------

//Estructura para la lista de pipes 
typedef struct {
    int p[2];
} tPipe;

//Estructura para la lista de lineas al bg (cada elem una linea de mandatos)
typedef struct {
    int *pidsLineEst; //Pids de la linea que guardamos dentro del elem
    int numPids; //Tamaño lista de pids
    int contTerminados; //Contador para comprobar el numero de procesos finalizados de la linea
    char *linea; //Guardamos la linea de mandatos en texto para mostrar con el jobs
} tBgElem;

tline * comando; //Linea de comandos en el parser
tPipe *pipes; //Array de pipes 

tBgElem *procBG; //Array de lineas en BG
int tamañoBG; //Tamaño lista de lineas en BG

int pid; //Pid temporal
int i; //Var contador (nmandatos())
int status; //status de un proceso hijo

char buf[1024];

//listas de pids 
pid_t *pidsLine; //Array de pids de la linea (si mandamos a BG copiar en la lista de tBgElem) 
pid_t* pidsFg; //Array de pids de linea en FG
int nFg; //numero de comando en Fg

mode_t mask; //guarda el numero de permisos que queremos quitar usando umask 
 
//Cabeceras de funciones
void nmandatos();// hecho

int mandatosInternos();
void cd();
void jobs(); //
void fg(int n);//
void calculaUmask();///hecho

void redirec();// hecho
void comprobacionZombies();
void reorganizar(int n);

//Manejador de señales 
int sig;

void manejador ();
void manejador2();

void manejador(){

    if (pidsFg!=0){
        for(int k=0; k<nFg; k++){
            kill(pidsFg[k],9);
        }
    }

}

void manejador2(){
    reorganizar(tamañoBG-1);
}


//-------------------------------------------------------------------------------------------------------------
//COMIENZO DEL PROGRAMA
//-------------------------------------------------------------------------------------------------------------
int main(){ 
    
    //Imprime lo que haya en el buffer de forma inmediata 
    setvbuf(stdout, NULL, _IONBF, 0);
    
    // Señales de control
    signal (SIGINT, manejador); //Evitar Ctrl + c 
    signal (SIGUSR1, manejador2); // Señal de error al hacer un comando -> eliminar lo que se haya guardado en el BG
    signal(SIGCHLD,comprobacionZombies); //Señal de hijo terminado -> entrar a comprobacionZombies

    //Inicializacion de variables
    pidsLine=0;
    pidsFg=0;
    tamañoBG=0;
    mask = 18; //18 en octal => 22 en decimal (mode_t guarda el numero en octal)  
    comando=0;
    
    printf("msh> ");

    while(fgets(buf, 1024, stdin)){
        comando = tokenize(buf); 

        //Si la linea esta vacia continuamos 
        if (comando->ncommands==0) {

            comando=0;
            printf("\nmsh> ");
            continue;

	    }

        //Si el mandato de la linea es interno
        if (mandatosInternos()){

            comando=0;
            printf("\nmsh> ");
            continue;

        }
        
        //Iniciamos el array de los pipes con el tamaño igual al numero de mandatos de la linea menos 1
        pipes = (tPipe *)calloc(comando->ncommands-1, sizeof(tPipe));

        //Iniciamos el array de pids de la linea con el tamaño igual al numero de mandatos de la linea
        pidsLine = (int *)calloc(comando->ncommands, sizeof(int));
        
        //Caso para un mandato en linea -> No hacemos ningun pipe
        if (comando->ncommands==1){

            pid=fork();
            i=0;
            pidsLine[i] = pid;
            
        }else{
            nmandatos();
        }
    
        if (pid==0){ //Si es el hijo

            signal (SIGINT, SIG_IGN);
            
            redirec(); //Comprueba si hay redirecciones en la linea 
                
            execvp(comando->commands[i].argv[0], comando-> commands[i].argv); //Crea el proceso hijo
            fprintf(stderr,"Error a la hora de hacer el comando\n");

            if (comando->background==1){ //Mandamos selal al padre para que elimine esta instruccion de la estructura del BG ya que no está bien
                kill(getppid(),SIGUSR1);
            }
            
            exit(1);

        }
        else{ //Si es el padre 

            if(comando->background==1){ //Si mandamos la linea al BG

                tamañoBG++;

                //Redimensionamos la lista de las lineas del BG
                procBG = (tBgElem *)realloc(procBG, tamañoBG*sizeof(tBgElem)); 

                //Guarda la linea de mandatos
                procBG[tamañoBG-1].linea = strdup (buf);  

                //Cambiar el & por \0
                procBG[tamañoBG-1].linea[strlen(procBG[tamañoBG-1].linea) - 2] = '\0'; 

                //Guarda pids de la linea en la lista del elem 
                procBG[tamañoBG-1].pidsLineEst = pidsLine; 

                //Guarda el num de mandatos de la linea en el numpids
                procBG[tamañoBG-1].numPids = comando->ncommands; 

                //Inicia contador de procesos terminados a cero 
                procBG[tamañoBG-1].contTerminados = 0; 

                //Imprimimos la posicion de la lista en jobs y los peides que entran 
                printf("[%d] ",tamañoBG); 
                for(int k=0; k<comando->ncommands;k++){
                    printf("%d ",pidsLine[k]);
                }
                printf("\n");

                //Lista de pids apunta a null
                pidsLine=0;  
            }
            else { //Si no mandamos la linea al BG

                //Guardamos los pids de la linea
                pidsFg=pidsLine; 
                nFg=comando->ncommands;
                pidsLine=0;

                for(int j =0; j<comando->ncommands;j++){
                    wait(NULL);
                }

                pidsFg=0;
                nFg=0;

            }
            
           
        }
        comando=0;
        printf("\nmsh> ");
    }

    //Terminamos procesos antes de terminar
    for(int k =0; k<tamañoBG; k++){
        for(int j = 0; j < procBG[k].numPids; j++){
            kill(procBG[k].pidsLineEst[j],9);
        }
    }

    free(pidsFg); 
    free(pidsLine); 
    free(pipes); 
    free(procBG); 
    return 0;

}

//-------------------------------------------------------------------------------------------------------------
//FUNCIONES AUXILIARES
//-------------------------------------------------------------------------------------------------------------
int mandatosInternos(){

    int true=0;

    if (strcmp(comando->commands[0].argv[0],"exit")==0) {//cuando tenemos exit or CTLR+D terminamos
        //Eliminamos los hijos que se han quedado en el BG
        for(int k =0; k<tamañoBG; k++){
            for(int j = 0; j < procBG[k].numPids; j++){
                kill(procBG[k].pidsLineEst[j],9);
            }
        }
        
        free(pidsFg); 
        free(pidsLine); 
        free(pipes); 
        free(procBG); 

        exit(0); 
    }
    if (strcmp(comando->commands[0].argv[0],"cd")==0) {
        cd();
        true=1;
    } 
    else if (strcmp(comando->commands[0].argv[0],"jobs")==0) {
        jobs();
        true=1;    
    } 
    else if (strcmp(comando->commands[0].argv[0],"fg")==0) {
        if (comando->commands[0].argc==1){
            fg(1);
        }else{
            fg(atoi(comando->commands[0].argv[1]));
        }
        true=1; 
    }
    else if (strcmp(comando->commands[0].argv[0],"umask")==0) {
        calculaUmask();
        true=1; 
    }
    return true;

}

//-------------------------------------------------------------------------------------------------------------
void cd(){

    char *dir; 
   
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

        //si salio mal el cambio de directorio
        if(chdir(dir) != 0){
            fprintf(stderr, "Error al cambiar de directorio\n");
        }
    }

    //si tiene el nombre y mas de un argumento 
    if (comando->commands[0].argc > 2){
        fprintf(stderr, "Numero de argumentos incorrecto para el cd\n");
    }

}

//-------------------------------------------------------------------------------------------------------------
void nmandatos(){

    for(i=0; i<comando->ncommands; i++){
        
        if (i==0){ //PRIMER MANDATO DE LA LINEA
        
            pipe(pipes[i].p); //Creamos el pipe
            pid=fork(); //Creamos proceso hijo
            pidsLine[i] = pid; //Guarda pid en la lista 

            if (pid < 0) { /* Error */
                fprintf(stderr, "Falló el fork().\n");
                exit(1);
            }

            if (pid==0){ //Somos el hijo
                
                close(pipes[i].p[0]); //Cerramos descriptor de lectura -> Solo queremos escribir
                dup2(pipes[i].p[1], STDOUT_FILENO); //Ponemos STDOUT en el pipe[1] 
                close(pipes[i].p[1]); //Cerramos el descriptor [1]
                break; //Salimos del for 

            }
            else{ //Somos el padre
                close(pipes[i].p[1]); //Cerramos descriptor de escritura 
            }
        }
        else if(i<comando->ncommands-1){ //MANDATOS INTERMEDIOS

            pipe(pipes[i].p); //Creamos siguiente pipe 		  
            pid = fork(); //Creamos el siguiente proceso 
            pidsLine[i] = pid; //Introducimos pid en la lista

            if (pid < 0) { /* Error */
                fprintf(stderr, "Falló el fork().\n");
                exit(1);
            } 

            if (pid==0){ //Somos el hijo
                
                close(pipes[i].p[0]); //Cerramos descriptor de lectura -> Solo queremos escribir

                dup2(pipes[i-1].p[0], STDIN_FILENO); //Cogemos la salida del pipe anterior y la ponemos como STDIN
                close(pipes[i-1].p[0]); //Cerramos ese descriptor del pipe 

                dup2(pipes[i].p[1], STDOUT_FILENO); //Ponemos STDOUT en el pipe[1] 
                close(pipes[i].p[1]); //Cerramos el descriptor [1]
                break;

            }
            else{ //Somos el padre
               
                close(pipes[i-1].p[0]); //Cerramos el descriptor de lectura del anterior pipe, no lo usamos 
                close(pipes[i].p[1]); //Cerramos el descriptor de escritura del nuevo pipe  
            
            }

        }
        else{ //ULTIMO MANDATO

            pid = fork(); 
            pidsLine[i] = pid;

            if (pid < 0) { /* Error */
                fprintf(stderr, "Falló el fork().\n");
                exit(1);
            }

            if (pid==0){ //Si somos el hijo
            
                dup2(pipes[i-1].p[0],STDIN_FILENO); //Cogemos la salida del pipe anterior y la ponemos como STDIN
                close(pipes[i-1].p[0]); //Cerramos ese descriptor del pipe 
                break;
            
            }
        }
        
    }

}

//-------------------------------------------------------------------------------------------------------------
void redirec(){

    if( (comando->ncommands==1) ||(i==0 || i==comando->ncommands-1 )){

        if (comando->redirect_input != NULL && i==0) {
            
            int fd=open(comando->redirect_input,  O_RDONLY);
            if (fd <0){
                fprintf(stderr, "Error al abrir el fichero\n");
                exit(1);
            }
            dup2(fd, 0);

        }
        //-----------------------------------------------------------------------------------------
        if (comando->redirect_output != NULL && i==comando->ncommands-1 ) {

            int fd=open(comando->redirect_output,  O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR| S_IWUSR | S_IRGRP|S_IROTH );
            
            if (fd <0){
                fprintf(stderr, "Error al abrir el fichero\n");
                exit(1);
            }
            dup2(fd, 1);
            
        }
        //-----------------------------------------------------------------------------------------
        if (comando->redirect_error != NULL && i==comando->ncommands-1) {
            int fd=open(comando->redirect_error,  O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR| S_IWUSR | S_IRGRP|S_IROTH );
            if (fd <0){
                fprintf(stderr, "Error al abrir el fichero\n");
                exit(1);
            }
            dup2(fd, 2);

        }
        //-----------------------------------------------------------------------------------------
    }

}

//-------------------------------------------------------------------------------------------------------------
void comprobacionZombies(){

    for(int k = 0; k < tamañoBG; k++){ //Recorre lista procBG
        for(int j = 0; j < procBG[k].numPids; j++){ //Recorre lista del elem de procBG, busca el que mando la señal
                
            //Comprobamos si termino el proceso 
            if(waitpid(procBG[k].pidsLineEst[j], &status, WNOHANG) == procBG[k].pidsLineEst[j]){
                //WIFEXITED y WEXITSTATUS se podrian quitar 
                //if (WIFEXITED(status) != 0){ //si termina de manera normal...
                    //if (WEXITSTATUS(status) == 0)
                        procBG[k].contTerminados++; //Aumenta contador de procesos terminados en esa linea
                //}
            }

        }

        //Si han terminado todos los procesos de esa linea
        if(procBG[k].contTerminados == procBG[k].numPids){ 
                
            printf("\n[%d]  + done      %s\n", k+1, procBG[k].linea);
            printf("\nmsh> ");
            
            //Reorganizamos las posiciones de la lista 
            reorganizar(k);
            break;

        }
    }

}

//-------------------------------------------------------------------------------------------------------------
void reorganizar(int n){

    free(procBG[n].pidsLineEst); //libera la mem de la lista de pids del elem eliminado 
    free(procBG[n].linea); //libera la mem de la lista que guardaba la linea 

    if(tamañoBG > 1){ //Si hay mas de un elem en la lista movemos los elementos para rellenar los huecos 

        for(int j=n; j<tamañoBG-1; j++){
            procBG[j]=procBG[j+1];
        }

    }
    tamañoBG--; //disminuimos el tamaño para redimensionar la lista despues 

}

//-------------------------------------------------------------------------------------------------------------
void jobs(){ //Imprime los procesos de la lista procBG

    for (int i = 0; i < tamañoBG; i++){
        printf("[%d]    running     %s\n", i+1, procBG[i].linea);
    }

}

//-------------------------------------------------------------------------------------------------------------
void fg(int n){ //Manda una linea que esta en BG al FG

    if (n<=tamañoBG){
        pidsFg=procBG[n-1].pidsLineEst;
        nFg=procBG[n-1].numPids;
        
        for (int j = 0; j <procBG[n-1].numPids; j++){
            waitpid(procBG[n-1].pidsLineEst[j],NULL,0);
        }
        procBG[n-1].pidsLineEst=0;

        pidsFg=0;
        nFg=0;
        reorganizar(n-1);
    }
    else{
        printf("Argumento incorrecto\n");
        exit(1);
    }

}

//-------------------------------------------------------------------------------------------------------------
void calculaUmask(){ 

    //si tiene solo el nombre 
    if (comando->commands[0].argc == 1){
        printf("%04o\n", mask); //imprimir el valor de la variable oldmask
    }

    //si tiene el nombre y una argumento
    if (comando->commands[0].argc == 2){
        sscanf(comando->commands[0].argv[1],"%ho",&mask); //guardamos el numero en la var mask 
        if(mask >= 0 && mask <= 0666){ 
            umask(mask); //llamamos a umask para que haga la resta 
        }
        else{
            fprintf(stderr, "El numero introducido es incorrecto\n");
            mask = 18; //Vuelve a asignar el valor por defecto 
        }
    }

    //si tiene el nombre y mas de un argumento 
    if (comando->commands[0].argc > 2){
        fprintf(stderr, "Numero de argumentos incorrecto para el umask\n");
    }

}