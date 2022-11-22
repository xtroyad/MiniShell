
typedef struct { // la direccion a la instruccion
	char * filename; 
	int argc; 	// de una instruccion el numero de argumentos
	char ** argv; // un puntero al mandato y los argumentos
} tcommand;

typedef struct {
	int ncommands; //Numero de instrucciones que hay, las redirecciones no cuentas
	tcommand * commands; //Un puntero a una lista de instrucciones que se han enviado separados por |
	char * redirect_input; //indica de donde se coge la entrada estandar
	char * redirect_output; //indica donde se direcciona la salida estandar
	char * redirect_error;// donde se redirecciona la salida de error
	int background;
} tline;

extern tline * tokenize(char *str);

