#include "Consola_S-AFA.h"
#include "S-AFA.h"
#include "FuncionesConexiones.h"
int cantHilosPLP = 0;
int cantidadDeCentinelas(char** centinelas){
	int i = 0;
	while(centinelas[i]){
		i++;
	}
	return i;
}

void consola() {
  t_log *logger_consola=log_create("Consola_S-AFA.log","Consola_S-AFA",1, LOG_LEVEL_INFO);
  char* linea = string_new();
  char** centinelas;
  log_info(logger_consola, "Gestor de programas G.DT");
  log_info(logger_consola, "Ingrese la operacion deseada:");
  log_info(logger_consola, "ejecutar RutaEncriptorio");
  log_info(logger_consola, "status (ID_DTBLOCK)");
  log_info(logger_consola, "finalizar ID_DTBLOCK");
  log_info(logger_consola, "metricas (ID_DTBLOCK)");

  while(1) {
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
    		if(!strcmp(centinelas[0],"status")){
    			printf("Usted ingreso status\n");
    			log_info(logger_consola, "Se ingreso comando status");
    			statusColasSistema();
    		}
    		else if(!strcmp(centinelas[0],"metricas")){
        		printf("Usted ingreso metricas\n");
        		log_info(logger_consola, "Se ingreso comando metricas");
    		}
    	    else{
    	    	printf("No se reconocio el comando %s\n", centinelas[0]);
    	    	log_error(logger_consola, "No se reconocio el comando");
    	    }
    		break;
    	case 2:
	   		if(!strcmp(centinelas[0],"ejecutar")){

	   			char *script=string_new();
	   			if(!string_starts_with(centinelas[1],"/"))
	   				string_append(&script,"/");

	   			string_trim(&(centinelas[1]));//quita espacios en blanco
	   			string_append(&script,centinelas[1]);
	    		printf("Usted ingreso ejecutar\n");
	    		printf("con la ruta encriptorio =  %s\n", script);
	    		log_info(logger_consola, "Se ingreso comando ejecutar");
	    		//probando
	    		pthread_t* hilosPLP = NULL;
	    		hilosPLP = realloc(hilosPLP,sizeof(pthread_t)*(cantHilosPLP+1));
	    		pthread_create(&(hilosPLP[cantHilosPLP]),NULL,planificadorLargoPlazo,(void*)script);
	    		cantHilosPLP++;
//	    	    planificadorLargoPlazo(script);
	    	}
    		else if(!strcmp(centinelas[0],"metricas")){
    		    printf("Usted ingreso metricas\n");
    		    printf("con ID_DTBLOCK =  %s\n", centinelas[1]);
    		    log_info(logger_consola, "Se ingreso comando metricas");
    		}
	   		else if(!strcmp(centinelas[0],"status")){
	        	printf("Usted ingreso status\n");
	        	printf("con ID_DTBLOCK =  %s\n", centinelas[1]);
	        	log_info(logger_consola, "Se ingreso comando status");
	        	string_trim(&(centinelas[1]));//quita espacios en blanco
	        	statusProceso(centinelas[1]);
	        	}
	   	    else if(!strcmp(centinelas[0],"finalizar")){
	   	    	printf("Usted ingreso finalizar\n");
	   	    	printf("con ID_DTBLOCK =  %s\n", centinelas[1]);
	   	    	log_info(logger_consola, "Se ingreso comando finalizar");
	   	    	string_trim(&(centinelas[1]));//quita espacios en blanco
	   	    	finalizarProceso(centinelas[1]);
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

