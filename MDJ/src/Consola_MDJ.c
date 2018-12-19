#include "Consola_MDJ.h"
#include "MDJ.h"
#include "FuncionesConexiones.h"

int cantidadDeCentinelas(char** centinelas){
	int i = 0;
	while(centinelas[i]){
		i++;
	}
	return i;
}

void* consola() {
  t_log *logger_consola=log_create("Consola_MDJ.log","Consola_MDJ",1, LOG_LEVEL_INFO);
  char *aux = string_new();
  t_config *config = config_create(pathMDJ);
  aux = config_get_string_value(config, "PUNTO_MONTAJE");
  puntoInicialConsola = string_new();
  puntoInicialConsola = string_substring(aux,0,string_length(aux)-1);
//  puntoInicialConsola = string_substring_until(puntoInicialConsola,string_length(aux)-1);
  log_info(logger,"punto inicial consola: %s", puntoInicialConsola);
  config_destroy(config);
  char* linea = string_new();
  char** centinelas;
  log_info(logger_consola, "Consola File System");
  log_info(logger_consola, "Ingrese la operacion deseada:");
  log_info(logger_consola, "ls (path_Directorio)");
  log_info(logger_consola, "cd (path_Directorio)");
  log_info(logger_consola, "md5 (path_Archivo)");
  log_info(logger_consola, "cat (path_Archivo)");
  pathActual = string_new();
  string_append(&pathActual,puntoInicialConsola);
  string_append(&pathActual,"/$  ");
  while(1) {
	log_info(logger,"pathActual: %s", pathActual);
    linea = readline(">");
    if (!linea) {
      break;
    }
//    int i =0;
    centinelas = string_split(linea," ");
    int n = cantidadDeCentinelas(centinelas);
    switch(n){
    	case 0:
    		break;
    	case 1:
    		if(!strcmp(centinelas[0],"ls")){
    			printf("Usted ingreso ls\n");
    			log_info(logger_consola, "Se ingreso comando ls");
    			ls(puntoInicialConsola);
    		}
    	    else{
    	    	printf("No se reconocio el comando %s\n", centinelas[0]);
    	    	log_error(logger_consola, "No se reconocio el comando");
    	    }
    		break;
    	case 2:
	   		if(!strcmp(centinelas[0],"ls")){

	   			char *script=string_new();
	   			if(!string_starts_with(centinelas[1],"/"))
	   				string_append(&script,"/");

	   			string_trim(&(centinelas[1]));//quita espacios en blanco
	   			string_append(&script,centinelas[1]);
	    		printf("Usted ingreso ls\n");
	    		printf("con el path del directorio =  %s\n", script);
	    		log_info(logger_consola, "Se ingreso comando ls");
	    		ls(centinelas[1]);
	    	}
    		else if(!strcmp(centinelas[0],"cd")){
    		    printf("Usted ingreso cd\n");
    		    printf("con el path del directorio =  %s\n", centinelas[1]);
    		    log_info(logger_consola, "Se ingreso comando cd");
    		    string_trim(&(centinelas[1]));//quita espacios en blanco
    		    log_info(logger_consola, "che..: %s" , centinelas[1]);
    		    cd(centinelas[1]);
    		}
	   		else if(!strcmp(centinelas[0],"md5")){
	        	printf("Usted ingreso md5\n");
	        	printf("con el path del archivo =  %s\n", centinelas[1]);
	        	log_info(logger_consola, "Se ingreso comando md5");
	        	string_trim(&(centinelas[1]));//quita espacios en blanco
	        	md5(centinelas[1]);
	        }
	   	    else if(!strcmp(centinelas[0],"cat")){
	   	    	printf("Usted ingreso cat\n");
	   	    	printf("con el path del archivo =  %s\n", centinelas[1]);
	   	    	log_info(logger_consola, "Se ingreso comando cat");
	   	    	string_trim(&(centinelas[1]));//quita espacios en blanco
	   	    	int size,flag;
	   	    	char* codigoEscriptorio;
	   	    	flag = validarArchivo2(centinelas[1], &size);
	   	    	if(flag != 0)
				{
					codigoEscriptorio = obtenerDatosArchivo2(centinelas[1],0,size);
					log_info(logger,"Codigo archivo: %s", codigoEscriptorio);
				}
	   	    	if(flag == 0)
	   	    	{
	   	    		char *auxiliar = string_new();
					string_append(&auxiliar,puntoInicialConsola);
					string_append(&auxiliar,"/");
					string_append(&auxiliar,centinelas[1]);
					log_info(logger,"aux: %s", auxiliar);
					flag = validarArchivo2(auxiliar, &size);
					if(flag != 0)
					{
						codigoEscriptorio = obtenerDatosArchivo2(auxiliar,0,size);
						log_info(logger,"Codigo archivo: %s", codigoEscriptorio);
					}
					else
					{
						log_info(logger, "El archivo no existe");
					}
	   	    	}

	   	    }
    	    else{
    	    	printf("No se reconocio el comando %s\n", centinelas[0]);
    	    	log_error(logger_consola, "No se reconocio el comando");
    	    }
    		break;
    	default:
    		printf("Usted ingreso una cantidad de argumentos invalida\n");
    		log_error(logger_consola, "Cantidad de argumentos invalida");
    }
//    log_destroy(logger);
    free(centinelas);
    free(linea);
  }
}


void ls_sinParametro(){
	struct dirent* pDirent;
		DIR* pdir = opendir(puntoInicialConsola);
		if(pdir == NULL){
			printf("No puedo abrir el directorio %s \n", puntoInicialConsola);
		}else{
			while((pDirent = readdir(pdir)) != NULL){
				printf("%s \n", pDirent->d_name);
			}
			closedir(pdir);
		}
}

void ls(char* pathDirectorio){
	struct dirent* pDirent;
	DIR* pdir = opendir(pathDirectorio);
	if(pdir == NULL){
		printf("No puedo abrir el directorio %s \n", pathDirectorio);
	}else{
		while((pDirent = readdir(pdir)) != NULL){
			printf("%s \n", pDirent->d_name);
		}
		closedir(pdir);
	}
}


void cd(char* pathDirectorio)
{
	if(chdir(pathDirectorio) == 0)
	{
		printf("aaa\n");
//		char* path = malloc(250);
//		getcwd(path, 250);
//		memcpy(puntoInicialConsola, path, strlen(path) + 1);
//		free(path);
		puntoInicialConsola = string_new();
		string_append(&puntoInicialConsola,pathDirectorio);
//		string_append(&puntoInicialConsola,"/");
		pathActual = string_new();
		string_append(&pathActual,puntoInicialConsola);
		string_append(&pathActual,"/$  ");
	}
	else
	{
		printf("Eeeee\n");
		char *aux = string_new();
		string_append(&aux,puntoInicialConsola);
		string_append(&aux,"/");
		string_append(&aux,pathDirectorio);
		if(chdir(aux) == 0)
		{
			printf("Eeiiiieee\n");
			string_append(&puntoInicialConsola,"/");
			string_append(&puntoInicialConsola,pathDirectorio);
			pathActual = string_new();
			string_append(&pathActual,puntoInicialConsola);
			string_append(&pathActual,"/$  ");
		}
		else
		{
			printf("Error, no pude entrar a ese path\n");
		}
	}
}


void md5(char* pathArchivo)
{
	int size,flag;
	char* codigoEscriptorio;
	flag = validarArchivo2(pathArchivo, &size);
	if(flag != 0)
	{
		codigoEscriptorio = obtenerDatosArchivo2(pathArchivo,0,size);
		log_info(logger,"Codigo archivo: %s", codigoEscriptorio);
		unsigned char digest[16];
		MD5_CTX context;
		MD5_Init(&context);
		MD5_Update(&context, codigoEscriptorio, strlen(codigoEscriptorio));
		MD5_Final(digest, &context);
		char out[33];
		for(int i = 0; i < 16; ++i)
		{
			sprintf(&out[i*2], "%02x", (unsigned int)digest[i]);
		}
		printf("%s\n", out);
	}
	if(flag == 0)
	{
		char *aux = string_new();
		string_append(&aux,puntoInicialConsola);
		string_append(&aux,"/");
		string_append(&aux,pathArchivo);
		flag = validarArchivo2(aux, &size);
		if(flag != 0)
		{
			codigoEscriptorio = obtenerDatosArchivo2(aux,0,size);
			log_info(logger,"Codigo archivo: %s", codigoEscriptorio);
			unsigned char digest[16];
			MD5_CTX context;
			MD5_Init(&context);
			MD5_Update(&context, codigoEscriptorio, strlen(codigoEscriptorio));
			MD5_Final(digest, &context);
			char out[33];
			for(int i = 0; i < 16; ++i)
			{
				sprintf(&out[i*2], "%02x", (unsigned int)digest[i]);
			}
			printf("%s\n", out);
		}
		else
		{
			log_info(logger, "El archivo no existe");
		}
	}
}

int validarArchivo2(char* nombreArchivo, int* size)
{
	FILE *archivo= fopen(nombreArchivo,"r");
	if(archivo == NULL)
	{
		log_info(logger, "Error 30004: El archivo no existe");
		return 0; //archivo inexistente
	}
	else
	{
		log_info(logger, "Eeeeee");
		t_config *configArchivo = config_create(nombreArchivo);  // /fifa-examples/fifa-checkpoint/Archivos/scripts/
		(*size) = config_get_int_value(configArchivo, "TAMANIO");
		log_info(logger, "oooo");
		config_destroy(configArchivo);
//		config_destroy(configMDJ);
		fclose(archivo);
	}
	log_info(logger,"Archivo validado exitosamente");
	return 1; //archivo existente
}

char* obtenerDatosArchivo2(char* nombreArchivo ,int offset, int size)
{
	log_info(logger,"Obteniendo datos archivo...");
	char* pathArchivo = string_new();
	string_append(&pathArchivo, nombreArchivo);
	t_config *configArchivo = config_create(pathArchivo);  // /fifa-examples/fifa-checkpoint/Archivos/scripts/
	char** bloques = config_get_array_value(configArchivo, "BLOQUES");
	int i = 0;
	char* codigoArchivo = string_new();
//	char* aux = malloc(tamanioBloque);
	log_info(logger,"Copiando info de cada bloque..");
	while(bloques[i] != NULL)
	{
		char* pathBloque = string_new();
		string_append(&pathBloque, puntoMontaje);
		string_append(&pathBloque, "Bloques/");  //    fifa-examples/fifa-checkpoint/Bloques/
		string_append(&pathBloque, bloques[i]);
		string_append(&pathBloque, ".bin");   //   fifa-examples/fifa-checkpoint/Bloques/0.bin
//		log_info(logger,"Obteniendo datos archivo... %s", pathBloque);
		FILE *archivo= fopen(pathBloque,"rb");
		fseek(archivo, 0, SEEK_END);
		int tam = ftell(archivo);
		char* aux = malloc(tam+1);
		fseek(archivo, 0, SEEK_SET);
		fread(aux,tam,1,archivo);
		aux[tam] = '\0';
		log_info(logger," datos archivo... %s", aux);
		string_append(&codigoArchivo, aux);
		free(aux);
		fclose(archivo);
		i++;
	}
	log_info(logger,"Carga de datos de archivo exitosa");
	char* codigoPedido = string_substring(codigoArchivo, offset, size);
	return codigoPedido;
}
