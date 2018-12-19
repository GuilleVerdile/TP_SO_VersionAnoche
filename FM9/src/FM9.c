#include "FuncionesConexiones.h"
#include "FM9.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>

void* conexionCPU(void* nuevoCliente){
  int sockCPU = *(int*)nuevoCliente;
  t_config *config=config_create(pathFM9);
  log_info(logger,"Estoy en el hilo del CPU");
  sem_post(&sem_esperaInicializacion);
  int flag ;
  while(1)
  {
//	  Paquete_DAM* paqueteDAM = malloc(sizeof(Paquete_DAM));
	  Paquete_DAM* paqueteDAM = inicializarPaqueteDam();
	  log_info(logger,"Esperando paquete del CPU..");
	  recibirPaqueteDAM(sockCPU,paqueteDAM);
	  log_info(logger,"Recibi un paqueteDAM del CPU..");
	  if(paqueteDAM->tipoOperacion == 7)  //Operacion Close
	  {
		  flag = liberarMemoria(paqueteDAM);
//		  en esta funcion estan los ifs para que libere segun el modo de la memoria usando el dato del paquete
//		  que le sea util (segmento,pagina,etc)
		  send(sockCPU,&flag,sizeof(int),0);
	  }
	  if(paqueteDAM->tipoOperacion == 3)  //Operacion Asignar
	  {
		  flag = asignarModificarArchivoEnMemoria(paqueteDAM); //splitear (*paqueteDAM).archivoAsociado.nombre en path linea datos, el path no se usa, se usa el segmento,pagina,etc.
		  send(sockCPU,&flag,sizeof(int),0);
	  }
	  if(paqueteDAM->tipoOperacion == 39)//pido sentencia
	  {
		  log_info(logger,"Buscando Sentencia para devolversela a CPU");
		  int nombreTam =0;
		  char* sentencia ;
		  char*modo= config_get_string_value(config, "MODO");
		  char* bloqueLinea ;
//			int tamanioLinea = config_get_int_value(config,"MAX_LINEA");
		  if(!strcmp(modo,"SEG"))
		  {
				int offset = paqueteDAM->archivoAsociado.programCounter;//numero de linea
				pthread_mutex_lock(&mutex_segmentoABuscar);
				numSegmentoAbuscar = paqueteDAM->archivoAsociado.numeroSegmento;
				SegmentoDeTabla *segmentoBuscado = list_find(lista_segmentos,&segmentoEsIdABuscar);
				pthread_mutex_unlock(&mutex_segmentoABuscar);
				if(segmentoBuscado->estaLibre == false && offset < ((segmentoBuscado->limite/tamanioMaxLinea)))
				{
//					log_info(logger,"hola");
//					int tam =segmentoBuscado->limite;
//					sentencia = malloc(tam +1);
//					memcpy(sentencia,memoriaReal + segmentoBuscado->base , tam);
//					sentencia[tam] = '\0';
					bloqueLinea = malloc(tamanioMaxLinea +1);
					log_info(logger,"Contenido en memoria: %s", mem_hexstring(memoriaReal, tamanioMaxLinea));
					char * posicion = memoriaReal +(((*segmentoBuscado).base + offset*tamanioMaxLinea));
					memcpy(bloqueLinea,posicion,tamanioMaxLinea);
					bloqueLinea[tamanioMaxLinea] = '\0';
					char ** lineas = string_split(bloqueLinea,"\n");
					if(lineas[0] != NULL)
					{
						nombreTam = string_length(lineas[0]);
						send(sockCPU,&nombreTam,sizeof(int),0);
						log_info(logger,"Sentencia: %s", lineas[0]);
						if(nombreTam){
							send(sockCPU,lineas[0],nombreTam,0);
						}
					}
					else
					{
						nombreTam = 0 ;
						send(sockCPU,&nombreTam,sizeof(int),0);  //ENVIAMOS NULL
					}
					log_info(logger,"Ya envie la sentencia");
				}
				if(offset == (segmentoBuscado->limite/tamanioMaxLinea))
				{
					nombreTam = 0 ;
					send(sockCPU,&nombreTam,sizeof(int),0);  //ENVIAMOS NULL
					log_info(logger,"Ya envie la sentencia: El escriptorio termino su ejecucion");
				}

		  }
		  else if(!strcmp(modo,"TPI")){
		  }
		  else if(!strcmp(modo,"SPA"))
		  {
			  	log_info(logger,"probando");
				pthread_mutex_lock(&mutex_idTablaABuscar);
				idTablaBuscar = paqueteDAM->idProcesoSolicitante;
				TablaSegmentosDeProceso* unaTablaSegmentosDeProceso = list_find(lista_procesos,&esProcesoAbuscar);
				pthread_mutex_unlock(&mutex_idTablaABuscar);
				pthread_mutex_lock(&mutex_segmentoPaginadoABuscar);
				numSegmentoPaginadoAbuscar = paqueteDAM->archivoAsociado.numeroSegmento;
				SegmentoPaginadoDelProceso* segmentoPaginado = list_find(unaTablaSegmentosDeProceso->segmentosDelProceso,&segmentoPaginadoEsIdABuscar);
				log_info(logger,"probando2");
				log_info(logger,"Proceso..: %d",(*segmentoPaginado).idProceso);
				log_info(logger,"Proceso..(seg): %d",(*segmentoPaginado).numeroSegmento);
				log_info(logger,"Proceso..(listsizepag): %d",list_size(segmentoPaginado->paginas));
				pthread_mutex_unlock(&mutex_segmentoPaginadoABuscar);
				int programCounter = paqueteDAM->archivoAsociado.programCounter;//numero de linea
				log_info(logger,"programCounter: %d ",programCounter);
				if(programCounter < list_size(segmentoPaginado->paginas))
				{
					char *script = string_new();
					for(int i = 0;i<list_size(segmentoPaginado->paginas);i++)
					{
						Pagina* paginaSegmento = list_get(segmentoPaginado->paginas,i);
						Marco* marco = list_get(lista_marcos,paginaSegmento->numeroMarco);
						log_info(logger,"Marco..: %d ",(*marco).numeroMarco);
						t_config *config2=config_create(pathFM9);
						int tamanioMaxPagina = config_get_int_value(config2,"TAM_PAGINA");
						sentencia = malloc(tamanioMaxPagina +1);
						memcpy(sentencia,memoriaReal + (*marco).direccionBase,tamanioMaxPagina);
						config_destroy(config2);
						sentencia[tamanioMaxPagina] = '\0';
						char ** lineas = string_split(sentencia,"\n");
						int m = 0;
						while(lineas[m] != NULL)
						{
							string_append(&script,lineas[m]);
							if(lineas[m+1] != NULL)
							{
								string_append(&script,"\n");
							}
							m++;
						}
						log_info(logger,"Sentencia: %s", script);
					}
					string_append(&script,"\0");  //puede que no vaya esto
					char ** lineasScript = string_split(script,"\n");
					int senteciaTam = string_length(lineasScript[programCounter]);
					send(sockCPU,&senteciaTam,sizeof(int),0);
					log_info(logger,"Sentencia: %s", lineasScript[programCounter]);
					if(senteciaTam){
						send(sockCPU,lineasScript[programCounter],senteciaTam,0);
						log_info(logger,"Ya envie la sentencia");
					}
					/*Pagina* paginaSegmento = list_get(segmentoPaginado->paginas,programCounter);
					Marco* marco = list_get(lista_marcos,paginaSegmento->numeroMarco);
					log_info(logger,"Marco..: %d ",(*marco).numeroMarco);
					t_config *config=config_create(pathFM9);
					int tamanioMaxPagina = config_get_int_value(config,"TAM_PAGINA");
					sentencia = malloc(tamanioMaxPagina +1);
					memcpy(sentencia,memoriaReal + (*marco).direccionBase,tamanioMaxPagina);
//					log_info(logger,"Sentencia: %s", sentencia);
					config_destroy(config);
					sentencia[tamanioMaxPagina] = '\0';
					char ** lineas = string_split(sentencia,"\n");
					int senteciaTam = string_length(lineas[0]);
					send(sockCPU,&senteciaTam,sizeof(int),0);
					log_info(logger,"Sentencia: %s", lineas[0]);
					if(senteciaTam){
						send(sockCPU,lineas[0],senteciaTam,0);
						log_info(logger,"Ya envie la sentencia");
					}*/
				}
				else  //Ya lei todas las paginas -> termino el escriptorio
				{
					int rta = 0;
					send(sockCPU,&rta,sizeof(int),0);  //ENVIAMOS NULL
					log_info(logger,"Ya envie la sentencia: El escriptorio termino su ejecucion");
				}

		  }

	  }
	  if(paqueteDAM->tipoOperacion == 50) //liberar memoria que ocupo el proceso ya que lleno la memoria de FM9
	  {
		  t_config *config=config_create(pathFM9);
		  	char*modo= config_get_string_value(config, "MODO");
		  	TablaSegmentosDeProceso* unaTablaSegmentosDeProceso;
		  	if(!strcmp(modo,"SEG"))
		  	{
		  		pthread_mutex_lock(&mutex_idTablaABuscar);
		  		idTablaBuscar = paqueteDAM->idProcesoSolicitante;
		  		unaTablaSegmentosDeProceso = list_find(lista_procesos,&esProcesoAbuscar);
		  		pthread_mutex_unlock(&mutex_idTablaABuscar);
		  		list_iterate(unaTablaSegmentosDeProceso->segmentosDelProceso, &liberarSegmento);
		  		for(int j = 0 ; j< list_size(unaTablaSegmentosDeProceso->segmentosDelProceso);j++)
				{
					list_remove(unaTablaSegmentosDeProceso->segmentosDelProceso,j);
				}
				list_destroy(unaTablaSegmentosDeProceso->segmentosDelProceso);
		  		log_info(logger,"Archivos Cerrado y liberados de memoria") ;
		  	}
		  	else if(!strcmp(modo,"TPI"))
		  	{

		  	}
		  	else if(!strcmp(modo,"SPA"))
		  	{
		  		pthread_mutex_lock(&mutex_idTablaABuscar);
		  		idTablaBuscar = paqueteDAM->idProcesoSolicitante;
		  		unaTablaSegmentosDeProceso = list_find(lista_procesos,&esProcesoAbuscar);
		  		pthread_mutex_unlock(&mutex_idTablaABuscar);
		  		list_iterate(unaTablaSegmentosDeProceso->segmentosDelProceso, &liberarSegmentoPaginado);
				for(int j = 0 ; j< list_size(unaTablaSegmentosDeProceso->segmentosDelProceso);j++)
				{
					list_remove(unaTablaSegmentosDeProceso->segmentosDelProceso,j);
				}
				list_destroy(unaTablaSegmentosDeProceso->segmentosDelProceso);
				log_info(logger,"Archivos Cerrado y liberados de memoria") ;
		  	}

	  }
	}
  return 0;
}

void liberarSegmento(SegmentoDelProceso *segmentoALiberar){
	log_info(logger,"Liberando Segmento...");
	pthread_mutex_lock(&mutex_segmentoABuscar);
	numSegmentoAbuscar = (*segmentoALiberar).numeroSegmento;
	SegmentoDeTabla *segmentoBuscado = list_find(lista_segmentos,&segmentoEsIdABuscar);
	list_remove_by_condition(lista_segmentos,&segmentoEsIdABuscar);
	pthread_mutex_unlock(&mutex_segmentoABuscar);
//	(*segmentoBuscado).estaLibre = true;
//	(*segmentoBuscado).limite =1;
	log_info(logger,"Segmento N° %d fue liberado de la memoria",(*segmentoALiberar).numeroSegmento);
//	free(segmentoBuscado);
}

void liberarSegmentoPaginado(SegmentoPaginadoDelProceso *segmentoPaginadoALiberar){
	log_info(logger,"Liberando Marcos asociados al Segmento Paginado...");
	list_iterate(segmentoPaginadoALiberar->paginas, &liberarMarcoAsociado);
	log_info(logger,"Segmento N° %d marcado ahora como libre",(*segmentoPaginadoALiberar).numeroSegmento);
}


void* conexionDAM(void* nuevoCliente){
	int sockDAM = *(int*)nuevoCliente;
	t_config *config=config_create(pathFM9);
	int transferSize = config_get_int_value(config,"TRANSFER_SIZE");//asignar tamaño
	sem_post(&sem_esperaInicializacion);
//	while(1)
//	{
//		Paquete_DAM* paqueteDAM = malloc(sizeof(Paquete_DAM));
		Paquete_DAM* paqueteDAM = inicializarPaqueteDam();

		recibirPaqueteDAM(sockDAM,paqueteDAM);
		log_info(logger,"Atendiendo PedidoDAM para proceso PID : N° %d",paqueteDAM->idProcesoSolicitante);
		int partes;
		if(paqueteDAM->tipoOperacion == 1)  //Operacion Abrir
		{
			char* buffer = malloc(transferSize* sizeof(char)+1);
			recv(sockDAM,&partes,sizeof(int),0);
			int numeroSegmento = 0;
			char* nuevoEscriptorio = string_new();//hay q mandar la linea con \0 incluido
			int transferSizeRestante;
			while(partes > 0)
			{
				if(partes == 1)
				{
					recv(sockDAM,&transferSizeRestante,sizeof(int),0);
					char* buffer2 = malloc(transferSizeRestante* sizeof(char)+1);
					recv(sockDAM,buffer2,transferSizeRestante,0);
					buffer2[transferSizeRestante] ='\0';
					string_append(&(nuevoEscriptorio), buffer2);
					free(buffer2);
				}
				else
				{
					recv(sockDAM,buffer,transferSize,0);
					buffer[transferSize] ='\0';
					string_append(&(nuevoEscriptorio), buffer);
				}
/*
				recv(sockDAM,buffer,transferSize,0);
				buffer[transferSize] ='\0';
				string_append(&(nuevoEscriptorio), buffer);
*/
				partes--;
				//cambiar para q funcione en caso de q el tamaño del escriptorio no sea multiplo del transfersize
			}
			free(buffer);
			log_info(logger,"Codigo archivo: %s", nuevoEscriptorio);
			char*modo= config_get_string_value(config, "MODO");
			if(!strcmp(modo,"SEG")){
				log_info(logger,"Agregando segmento...");
				numeroSegmento = agregarSegmento(nuevoEscriptorio,paqueteDAM);

			}
			else if(!strcmp(modo,"TPI"))
			{

			}
			else if(!strcmp(modo,"SPA"))
			{
				numeroSegmento = agregarSegmentoPaginado(nuevoEscriptorio,paqueteDAM);

			}

			if(numeroSegmento == 20003)
			{
				log_info(logger,"No se pudo agregar el segmento");
				paqueteDAM->resultado = 0;
			}
			else if(numeroSegmento == 10002)
			{
				log_info(logger,"No se pudo agregar el segmento");
				paqueteDAM->resultado = 10002;
			}
			else
			{
				log_info(logger,"Segmento agregado exitosamente");
				paqueteDAM->resultado = 1;
//					paqueteDAM->archivoAsociado.numeroSegmento = numeroSegmento;
			}
			enviarPaqueteDAM(sockDAM, paqueteDAM);
		}
		if(paqueteDAM->tipoOperacion == 6)  //Operacion Flush
		{
			char* info;
			char*modo= config_get_string_value(config, "MODO");
			if(!strcmp(modo,"SEG"))
			{
				info = obtenerInfoSegmento(paqueteDAM->archivoAsociado.numeroSegmento,paqueteDAM->idProcesoSolicitante);
			}
			else if(!strcmp(modo,"TPI")){

			}
			else if(!strcmp(modo,"SPA")){
				info = obtenerInfoSegmentoPaginado(paqueteDAM->archivoAsociado.numeroSegmento,paqueteDAM->idProcesoSolicitante);
			}
			log_info(logger,"Codigo obtenido del archivo: %s", info);
			partes = string_length(info)/transferSize;
			if(string_length(info) % transferSize != 0){
				partes++;
			}
			log_info(logger,"Partes: %d", partes);
			send(sockDAM,&partes,sizeof(int),0);
			char* aux = string_new();
			int i = 0 ;
			while(partes > 0)
			{
				if(partes == 1)
				{
					int resto = (string_length(info)+1) % transferSize ;
					if(resto != 0)
					{
						char* aux2 = string_new();
						send(sockDAM,&resto,sizeof(int),0);
						aux2 = string_substring(info,i*transferSize,resto);
						send(sockDAM,aux2,resto,0);
						free(aux2);
					}
					else
					{
						send(sockDAM,&transferSize,sizeof(int),0);
						aux = string_substring(info,i*transferSize,transferSize);
						send(sockDAM,aux,transferSize,0);
					}
				}
				else
				{
					aux = string_substring(info,i*transferSize,transferSize);
					send(sockDAM,aux,transferSize,0);
				}
				partes--;
				i++;
				log_info(logger,"Partes restantes..: %d", partes);
			  }

//			int partesAenviar = sizeof(info)/transferSize;
//			int resto = transferSize ;
//			if (sizeof(info)%transferSize != 0)
//			{
//				resto = sizeof(info)%transferSize;
//				partesAenviar++;
//				//cambiar para q funcione en caso de q el tamaño del escriptorio no sea multiplo del transfersize
//			}
//			char *enviar = malloc(sizeof(char)*transferSize);
//			int i = 0;
//			enviarPaqueteDAM(sockDAM, paqueteDAM);
//			send(sockDAM,&partesAenviar,sizeof(int),0);
//			while(partes > 0)
//			{
//				if((partes == 1) && (resto != 0))
//				{
//					memcpy(enviar, (info+i*transferSize),resto);
//				}
//				else
//				{
//					memcpy(enviar, (info+i*transferSize),transferSize);
//				}
//				send(sockDAM,enviar,transferSize,0);
//				partes--;
//				i++;
//			}
//			free(enviar);
		}



		liberarPaqueteDAM(paqueteDAM);
//	}
		config_destroy(config);
	close(sockDAM);
	return 0;
}


int main(void) {
//    t_log * logger = log_create("LOG_MEMORIA", "PROCESO FM9", 1, LOG_LEVEL_TRACE);
//    log_trace(logger, "Arranca la Memoria");
//	t_config *arcConfig = leerArcConfig();
//	int32_t retardo;
//	int32_t tamMemPpal;
//	int32_t tamMaximoLinea;
//	guardarDatosConfigFM9(arcConfig, &modoEjecucion,&tamMemPpal,&tamMaximoLinea);
//	crear_memoria();
	int cantidadDeCPUs = 0 ;
	int cantidadPedidosDAM = 0 ;
	logger = log_create(logFM9,"FM9",1, LOG_LEVEL_INFO);
	t_config *config=config_create(pathFM9);
	tamanioMemoria = config_get_int_value(config, "TAMANIO");
	int tamMaxPagina = config_get_int_value(config, "TAM_PAGINA");
	tamanioMaxLinea = config_get_int_value(config, "MAX_LINEA");
	memoriaReal = malloc(tamanioMemoria);	//STORAGE O MEMORIAREAL
    char* modoEjecucion;
    sem_init(&sem_esperaInicializacion,0,0);
    pthread_mutex_init(&mutex_segmentoABuscar,NULL);
    pthread_mutex_init(&mutex_segmentoPaginadoABuscar,NULL);
    pthread_mutex_init(&mutex_idTablaABuscar,NULL);
    pthread_mutex_init(&mutex_ListaMarcos,NULL);
    pthread_mutex_init(&mutex_direccionBaseAusar,NULL);
    pthread_mutex_init(&mutex_infoArchivoFlush,NULL);
    lista_procesos = list_create();
	char*modo= config_get_string_value(config, "MODO");
	pthread_t hilo_consola;
	pthread_create(&hilo_consola,NULL,(void *)consola,NULL);
	if(!strcmp(modo,"SEG")){
		lista_segmentos = list_create();
//		direccionBaseProxima = memoriaReal; //le asigno la primera posicion de memoria de la MP

		idGlobal = 0;
	}
	else if(!strcmp(modo,"TPI")){

	}
	else if(!strcmp(modo,"SPA")){
		idMarcoGlobal = 0;
		idGlobal = 0;
		cantidadMaximaMarcos = tamanioMemoria/tamMaxPagina;
		lista_marcos = list_create();
	}
	//crear hilo consola FM9

//    int sockDAM=crearConexionCliente(pathDAM);
//      if(sockDAM<0){
//    	  log_error(logger,"Error en la conexion con DAM");
//    	  return 1;
//      }
//      log_info(logger,"Se realizo correctamente la conexion DAM");
//      send(sockDAM,"3",2,0);
//      //hiloDAM
//      pthread_create(&hiloDAM,NULL,conexionDAM,(void*)&sockDAM);
      int sockFM9 = crearConexionServidor(pathFM9);
      if (listen(sockFM9, 10) == -1){ //ESCUCHA!
        log_error(logger, "No se pudo escuchar");
        log_destroy(logger);
        return -1;
      }
      pthread_t* hilosCPUs = NULL;
      pthread_t* hilosPedidosDAM = NULL;
      log_info(logger, "Se esta escuchando");
      char* buff;
      int sockCliente;
    while(sockCliente = accept(sockFM9, NULL, NULL)){
      if (sockCliente == -1) {
          log_info(logger,"error when accepting connection");
          exit(1);
        } else {
          buff = malloc(2);
          recv(sockCliente,buff,2,0);
          log_info(logger, "Llego una nueva conexion");
          if(buff[0] == '1'){
			  log_info(logger,"Se realizo correctamente la conexion con CPU");
			  hilosCPUs = realloc(hilosCPUs,sizeof(pthread_t)*(cantidadDeCPUs+1));
			  pthread_create(&(hilosCPUs[cantidadDeCPUs]),NULL,conexionCPU,(void*)&sockCliente);
			  cantidadDeCPUs++;
          }
          else if(buff[0] == '3'){
        	  log_info(logger,"Llego una nueva peticion del DAM");
        	  hilosPedidosDAM = realloc(hilosPedidosDAM,sizeof(pthread_t)*(cantidadPedidosDAM+1));
			  pthread_create(&(hilosPedidosDAM[cantidadPedidosDAM]),NULL,conexionDAM,(void*)&sockCliente);
			  cantidadPedidosDAM++;
          }
          free(buff);
          sem_wait(&sem_esperaInicializacion);
        }
    }
      sleep(1);
//      close(sockDAM);
      close(sockCliente);
      close(sockFM9);

  return EXIT_SUCCESS;
}


int agregarSegmento(char *nuevoEscriptorio,Paquete_DAM* paqueteDAM)
{
	log_info(logger,"Contenido segmento: %s", mem_hexstring(memoriaReal, string_length(nuevoEscriptorio)));

	int idProcesoSolicitante = paqueteDAM->idProcesoSolicitante;
//	SegmentoDeTabla *nuevoSegmento = malloc(sizeof(SegmentoDeTabla));
//	(*nuevoSegmento).estaLibre = false;
	log_info(logger,"Codigo script: %s", nuevoEscriptorio);
	int numSegmentoNuevo;
	char ** lineas = string_split(nuevoEscriptorio, "\n");
	int i =0;
	if(lineas[0] != NULL)
	{
		while(lineas[i++] != NULL); --i; //i tiene la cantidad de lineas
	}
	else
	{
		i = string_length(nuevoEscriptorio);
	}
	log_info(logger,"Cant stringlength: %d", string_length(nuevoEscriptorio));
	log_info(logger,"Cant Lineas: %d", i);
	int resultadoOperacion = dameProximaDireccionBaseDisponible(string_length(nuevoEscriptorio),&numSegmentoNuevo,i);
	log_info(logger,"numeroNuevoSegmento: %d",numSegmentoNuevo);
	if (resultadoOperacion == -1){
		log_info(logger,"Espacio insuficiente en FM9");
		return 10002;
	}
	if (resultadoOperacion == -2){
		log_info(logger,"Fallo de segmento");
		return 20002;
	}
	paqueteDAM->archivoAsociado.numeroSegmento = numSegmentoNuevo; //direccion segmento
	SegmentoDelProceso *nuevoSegmentoDelProceso = malloc(sizeof(SegmentoDelProceso));
	(*nuevoSegmentoDelProceso).base = resultadoOperacion; //direccion base posta
	//	char *mem_hexstring(void *source, size_t length);

	//DUMP FM9
	log_info(logger,"Direccion Base: %d", (*nuevoSegmentoDelProceso).base);

	//	(*nuevoSegmentoDelProceso).limite = string_length(nuevoEscriptorio)+1; //el limite es la cantidad maximas de lineas que tiene

	(*nuevoSegmentoDelProceso).limite = i * tamanioMaxLinea; //el limite es excluyente
	//*************************************************************************************
	//lo guarda completo por partes
	int count =0;
	char * infoArchivo;
	if(lineas[0] != NULL){//no es un crear
		while(count < i){
			infoArchivo = string_new();
			string_append(&infoArchivo,lineas[count]);
			string_append(&infoArchivo,"\n");
			log_info(logger,"Linea prox enn Memoria: %s",infoArchivo);
			memcpy(memoriaReal + ((*nuevoSegmentoDelProceso).base+count*tamanioMaxLinea),infoArchivo, string_length(infoArchivo));
			count++;
		}
	}
	else
	{
		while(count < i){
			infoArchivo = string_new();
			string_append(&infoArchivo,"\n");
			memcpy(memoriaReal + ((*nuevoSegmentoDelProceso).base+count*tamanioMaxLinea),infoArchivo, string_length(infoArchivo));
			count++;
		}
	}
		//estaba esto
	//	memcpy(memoriaReal + (*nuevoSegmentoDelProceso).base, nuevoEscriptorio,(*nuevoSegmentoDelProceso).limite); //guardo el escriptorio en la memoria real en forma de bits sin \0
	//
	//	******************************************************************************************
	log_info(logger,"Contenido segmento: %s", mem_hexstring(memoriaReal + (*nuevoSegmentoDelProceso).base, (*nuevoSegmentoDelProceso).limite));
	(*nuevoSegmentoDelProceso).idProceso = idProcesoSolicitante;
	(*nuevoSegmentoDelProceso).numeroSegmento = numSegmentoNuevo;
	pthread_mutex_lock(&mutex_idTablaABuscar);
	TablaSegmentosDeProceso *unaTablaSegmentosDeProceso;
	idTablaBuscar = idProcesoSolicitante;
	unaTablaSegmentosDeProceso = list_find(lista_procesos,&esProcesoAbuscar);
	pthread_mutex_unlock(&mutex_idTablaABuscar);
	if(unaTablaSegmentosDeProceso == NULL) //el proceso es nuevo, no tenia ningun segmento asociado
	{
		TablaSegmentosDeProceso *nuevaTablaSegmentosDeProceso = malloc (sizeof(TablaSegmentosDeProceso));
		(*nuevaTablaSegmentosDeProceso).idProceso = idProcesoSolicitante;
		(*nuevaTablaSegmentosDeProceso).segmentosDelProceso = list_create();
		list_add((*nuevaTablaSegmentosDeProceso).segmentosDelProceso,nuevoSegmentoDelProceso);
		list_add(lista_procesos,nuevaTablaSegmentosDeProceso);  //ANALIZANDOLO..
	}
	else
	{
		list_add((*unaTablaSegmentosDeProceso).segmentosDelProceso,nuevoSegmentoDelProceso);
	}
	return 1;
}

char *mem_hexstring(void *source, size_t length) {
  char *dump = string_new();
  // The dump_length should be the closes multiple of HEXDUMP_COLS after length
  unsigned int dump_length = length, mem_index = 0;
  if(length % HEXDUMP_COLS) {
    dump_length += HEXDUMP_COLS - length % HEXDUMP_COLS;
  }

  while(mem_index < dump_length)  {
    // Adds initial offset (0x00000: )
    if (mem_index % HEXDUMP_COLS == 0) {
      string_append_with_format(&dump, "\n0x%08x: ", mem_index);
    }
    // Adds hex data (00 00 00 00 00...)
    if (mem_index < length) {
      string_append_with_format(&dump, "%02x ", 0xFF & ((char *)source)[mem_index]);
    } else { // No more blocks to dump, so it adds 00
      string_append(&dump, "00 ");
    }
    // Adds an extra space if hex data is the last column of HEXDUMP_COLS_SEP
    if (mem_index % HEXDUMP_COLS_SEP == (HEXDUMP_COLS_SEP -1)) {
      string_append(&dump, " ");
    }
    // Adds ASCII dump if last column
    if (mem_index % HEXDUMP_COLS == (HEXDUMP_COLS - 1)) {
      unsigned int ascii_iterator = mem_index - (HEXDUMP_COLS - 1);
      string_append(&dump, "|");
      while(ascii_iterator <= mem_index) {
        if (ascii_iterator >= length) { // No more blocks to dump, so it adds .
          string_append(&dump, ".");
        } else if (isprint(((char *)source)[ascii_iterator])) { // Is printable char
          string_append_with_format(&dump, "%c", 0xFF & ((char *)source)[ascii_iterator]);
        }
        else { // Non printable chars
          string_append(&dump, ".");
        }
        ascii_iterator++;
      }
      string_append(&dump, "|");
    }
    mem_index++;
  }
  return dump;
}

int dameProximaDireccionBaseDisponible(int cantBytes, int *numSegmentoNuevo, int lineas){
	int baseSegmento, auxBase;
	SegmentoDeTabla *unSegmento = malloc (sizeof(SegmentoDeTabla));
	pthread_mutex_lock(&mutex_direccionBaseAusar);
	tamanioArchivoAMeter = cantBytes;
	direccionBaseAusar = 0;
	unSegmento = list_find(lista_segmentos,&entraArchivoEnSegmento);
	auxBase = direccionBaseAusar;
	pthread_mutex_unlock(&mutex_direccionBaseAusar);
	if(unSegmento == NULL)
	{
		log_info(logger,"auxBase + cantBytes: %d", auxBase + cantBytes);
		log_info(logger,"base segmento + limite: %d", auxBase + lineas * tamanioMaxLinea);  //ESTA ES LA POSICION FINAL EN MEMORIA DEL SEGMENTO (NO PUEDE SUPERAR EL TAMANIO MAXIMO DE MEMORIA DE FM9)
		log_info(logger,"tamanioMemoria: %d", tamanioMemoria);
		if(auxBase + lineas * tamanioMaxLinea <= tamanioMemoria)
		{
			baseSegmento = auxBase;
			SegmentoDeTabla *segmentoNuevoEntero = malloc (sizeof(SegmentoDeTabla));
			idGlobal++;
			(*segmentoNuevoEntero).base = baseSegmento;
//			(*segmentoNuevoEntero).limite = lineas;
			(*segmentoNuevoEntero).estaLibre = false;
//			(*segmentoNuevoEntero).CantBytes = cantBytes;
			(*segmentoNuevoEntero).limite = lineas * tamanioMaxLinea;  //EL ARCHIVO NUNCA VA A VARIAR EN CANTIDAD DE LINEAS
			log_info(logger,"Tamanio Segmento Posta: %d", (*segmentoNuevoEntero).limite);
			(*segmentoNuevoEntero).numeroSegmento = idGlobal;
			list_add(lista_segmentos,segmentoNuevoEntero);
			(*numSegmentoNuevo) = (*segmentoNuevoEntero).numeroSegmento;
		}
		else  //no entra
		{

			return -1;
		}
	}
	else
	{
		if(cantBytes > (*unSegmento).limite)
		{
			log_info(logger,"Error 999: Fallo de Segmento");
			return -2;
		}
//		numSegmentoAbuscar = (*unSegmento).numeroSegmento;
		baseSegmento = (*unSegmento).base;

		/*
		if(cantBytes != (*unSegmento).CantBytes)
		{
			SegmentoDeTabla *segmentoNuevo = malloc (sizeof(SegmentoDeTabla));
			idGlobal++;
			(*segmentoNuevo).base = baseSegmento;
			(*segmentoNuevo).limite = lineas;
			(*segmentoNuevo).estaLibre = false;
			(*segmentoNuevo).CantBytes = cantBytes;
			(*segmentoNuevo).numeroSegmento = idGlobal;
			list_add(lista_segmentos,segmentoNuevo);
			(*numSegmentoNuevo) = (*segmentoNuevo).numeroSegmento;
			SegmentoDeTabla *segmentoNuevo2 = malloc (sizeof(SegmentoDeTabla));
			idGlobal++;
			(*segmentoNuevo2).base = (*segmentoNuevo).base + (*segmentoNuevo).CantBytes ;
			(*segmentoNuevo2).limite = 0;
			(*segmentoNuevo).CantBytes = (*unSegmento).CantBytes - cantBytes; //cantidad de Bytes disponibles
			(*segmentoNuevo2).estaLibre = true ;
			(*segmentoNuevo2).numeroSegmento = idGlobal;
			list_add(lista_segmentos,segmentoNuevo2);
			list_remove_by_condition(lista_segmentos,&segmentoEsIdABuscar);
		}
		else
		{
			(*unSegmento).estaLibre = false;
			(*numSegmentoNuevo) = (*unSegmento).numeroSegmento;
		}
		*/
	}
	return baseSegmento;  //no importa el numero
}

bool entraArchivoEnSegmento(void *segmentoDeTabla){
	SegmentoDeTabla *b=(SegmentoDeTabla*) segmentoDeTabla;
	direccionBaseAusar = (*b).base ;
	if(((*b).limite >= tamanioArchivoAMeter) && ((*b).estaLibre == true)){
		return true;
	}
	else
		direccionBaseAusar = (*b).base + (*b).limite ;
		return false;
}

bool segmentoEsIdABuscar(void * segmento){
	SegmentoDeTabla *seg=(SegmentoDeTabla*) segmento;
	if((*seg).numeroSegmento==numSegmentoAbuscar)
		return true;
	else
		return false;
}

bool segmentoPaginadoEsIdABuscar(void * segmento){
	SegmentoPaginadoDelProceso *seg=(SegmentoPaginadoDelProceso*) segmento;
	if((*seg).numeroSegmento==numSegmentoPaginadoAbuscar)
		return true;
	else
		return false;
}


bool esProcesoAbuscar(void * proceso){

	TablaSegmentosDeProceso * tabla = (TablaSegmentosDeProceso *) proceso;
	if((*tabla).idProceso==idTablaBuscar)
		return true;
	else
		return false;
}

int liberarMemoria(Paquete_DAM* paqueteDAM)
{
	t_config *config=config_create(pathFM9);
	char*modo= config_get_string_value(config, "MODO");
	TablaSegmentosDeProceso* unaTablaSegmentosDeProceso;
	if(!strcmp(modo,"SEG"))
	{
		pthread_mutex_lock(&mutex_idTablaABuscar);
		idTablaBuscar = paqueteDAM->idProcesoSolicitante;
		unaTablaSegmentosDeProceso = list_find(lista_procesos,&esProcesoAbuscar);
		pthread_mutex_unlock(&mutex_idTablaABuscar);
		pthread_mutex_lock(&mutex_segmentoABuscar);
		numSegmentoAbuscar = paqueteDAM->archivoAsociado.numeroSegmento;
		list_remove_by_condition(unaTablaSegmentosDeProceso->segmentosDelProceso,&segmentoEsIdABuscar);
		SegmentoDeTabla *segmentoBuscado = list_find(lista_segmentos,&segmentoEsIdABuscar);
		pthread_mutex_unlock(&mutex_segmentoABuscar);
		if(segmentoBuscado == NULL){ //estaba cerrado
			return 40001;
		}
		(*segmentoBuscado).estaLibre = true;
		(*segmentoBuscado).limite =1;
		log_info(logger,"Archivo Cerrado y liberado de memoria") ;
		//falta ver cuando nos puede tirar 40002: Fallo de segmento/memoria.
		return 1;

	}
	else if(!strcmp(modo,"TPI"))
	{

	}
	else if(!strcmp(modo,"SPA"))
	{
		pthread_mutex_lock(&mutex_idTablaABuscar);
		idTablaBuscar = paqueteDAM->idProcesoSolicitante;
		unaTablaSegmentosDeProceso = list_find(lista_procesos,&esProcesoAbuscar);
		pthread_mutex_unlock(&mutex_idTablaABuscar);
		pthread_mutex_lock(&mutex_segmentoPaginadoABuscar);
		numSegmentoPaginadoAbuscar = paqueteDAM->archivoAsociado.numeroSegmento;
		SegmentoPaginadoDelProceso* segmentoPaginado = list_find(unaTablaSegmentosDeProceso->segmentosDelProceso,&segmentoPaginadoEsIdABuscar);
		list_remove_by_condition(unaTablaSegmentosDeProceso->segmentosDelProceso,&segmentoPaginadoEsIdABuscar);
		//finalmente borras el segmento que tenia referencia a ese archivo
		pthread_mutex_unlock(&mutex_segmentoPaginadoABuscar);
		list_iterate(segmentoPaginado->paginas, &liberarMarcoAsociado);

	}
	return 1;
}

void liberarMarcoAsociado(void * pagina){
	Pagina* paginaSegmento = (Pagina*) pagina;
	Marco* marco = list_get(lista_marcos,paginaSegmento->numeroMarco);
	(*marco).estaLibre = true;
}

int asignarModificarArchivoEnMemoria(Paquete_DAM* paqueteDAM){ //splitear (*paqueteDAM).archivoAsociado.nombre en path linea datos, el path no se usa, se usa el segmento,pagina,etc.

	t_config *config=config_create(pathFM9);
	char *nombraArchivoAescribir = string_new();
	char *lineaAescribir = string_new();
	char* datoAescribir = string_new();
	char *sentencia= string_new();
	string_append(&sentencia, paqueteDAM->archivoAsociado.nombre);
	string_trim(&sentencia);
	log_info(logger,"probando asignar1..");
	log_info(logger,"sentencia.. %s" , sentencia);
	log_info(logger,"idGlobal.. %d" , idGlobal);
	char** datos = string_n_split(sentencia, 3, " " );
	string_append(&nombraArchivoAescribir, datos[0]);
	string_append(&lineaAescribir, datos[1]);
	string_append(&datoAescribir, datos[2]);
	int numLineaACambiar = atoi(lineaAescribir);

	numLineaACambiar--; //PORQUE LAS LINEAS DEL ARCHIVO ARRANCAN EN 1

	char*modo= config_get_string_value(config, "MODO");
	if(!strcmp(modo,"SEG"))
	{
		pthread_mutex_lock(&mutex_idTablaABuscar);
		idTablaBuscar = paqueteDAM->idProcesoSolicitante;
		TablaSegmentosDeProceso* unaTablaSegmentosDeProceso = list_find(lista_procesos,&esProcesoAbuscar);
		pthread_mutex_unlock(&mutex_idTablaABuscar);
		log_info(logger,"probando asignar2..");
		pthread_mutex_lock(&mutex_segmentoABuscar);
		numSegmentoAbuscar = paqueteDAM->archivoAsociado.numeroSegmento;
		log_info(logger,"numSegmentoAbuscar..  %d", paqueteDAM->archivoAsociado.numeroSegmento);
//		list_remove_by_condition(unaTablaSegmentosDeProceso->segmentosDelProceso,&segmentoEsIdABuscar);
		log_info(logger,"probando asignar2-4..");
		log_info(logger,"numSegmentoAbuscar..  %d",numSegmentoAbuscar);
		SegmentoDeTabla *segmentoBuscado = list_find(lista_segmentos,&segmentoEsIdABuscar);
		pthread_mutex_unlock(&mutex_segmentoABuscar);
		string_append(&datoAescribir,"\n");

		if(string_length(datoAescribir) > tamanioMaxLinea){
			return 20002;
		}

		memcpy(memoriaReal + ((*segmentoBuscado).base + numLineaACambiar*tamanioMaxLinea),datoAescribir,string_length(datoAescribir));
		log_info(logger,"Contenido segmento PostAsignar: %s", mem_hexstring(memoriaReal + (*segmentoBuscado).base, (*segmentoBuscado).limite));

		//---------------------
		/*
		char* viejoEscriptorio = malloc((*segmentoBuscado).CantBytes+1);
		log_info(logger,"probando asignar2-5..");
		memcpy(viejoEscriptorio,memoriaReal + (*segmentoBuscado).base,(*segmentoBuscado).CantBytes);
		viejoEscriptorio[(*segmentoBuscado).CantBytes] = '\0';
		log_info(logger,"Codigo archivo preAsignar: %s",viejoEscriptorio);
		char ** lineas = string_split(viejoEscriptorio, "\n");
		log_info(logger,"cant lineas: %s", &lineas);
		log_info(logger,"probando asignar3..");
		int i =0;
		char* nuevoEscriptorio = string_new();
		while(lineas[i]!=NULL){

			if(numLineaACambiar != i){
				string_append(&nuevoEscriptorio, lineas[i]);
			}else
			{
				string_append(&nuevoEscriptorio, datoAescribir);
			}

			string_append(&nuevoEscriptorio,"\n");
			i++;
		}

		log_info(logger,"Codigo archivo postAsignar: %s",nuevoEscriptorio);

		segmentoBuscado->estaLibre =true;
		int numeroSegmentoOriginal = paqueteDAM->archivoAsociado.numeroSegmento;
		int flag = agregarSegmento(nuevoEscriptorio,paqueteDAM);

		cambiarNumeroSegmento(paqueteDAM,numeroSegmentoOriginal);

		return flag;

//		int tamanioLinea = config_get_int_value(config,"MAX_LINEA");

//		memcpy(memoriaReal + (*segmentoBuscado).base + paqueteDAM->archivoAsociado.programCounter,datoAescribir ,tamanioLinea );

		*/
	}
	else if(!strcmp(modo,"SPA"))
	{
		pthread_mutex_lock(&mutex_idTablaABuscar);
		idTablaBuscar = paqueteDAM->idProcesoSolicitante;
		TablaSegmentosDeProceso* unaTablaSegmentosDeProceso = list_find(lista_procesos,&esProcesoAbuscar);
		pthread_mutex_unlock(&mutex_idTablaABuscar);
		pthread_mutex_lock(&mutex_segmentoPaginadoABuscar);
		numSegmentoPaginadoAbuscar = paqueteDAM->archivoAsociado.numeroSegmento;
		SegmentoPaginadoDelProceso *segmentoPaginadoBuscado = list_find((*unaTablaSegmentosDeProceso).segmentosDelProceso,&segmentoEsIdABuscar);
		pthread_mutex_unlock(&mutex_segmentoPaginadoABuscar);
		int cantActualMarcos = list_size(segmentoPaginadoBuscado->paginas);
	/*	pthread_mutex_lock(&mutex_segmentoPaginadoABuscar);
		numSegmentoPaginadoAbuscar = paqueteDAM->archivoAsociado.numeroSegmento;
		SegmentoPaginadoDelProceso *segmentoPaginadoBuscado = list_find((*unaTablaSegmentosDeProceso).segmentosDelProceso,&segmentoEsIdABuscar);
		pthread_mutex_unlock(&mutex_segmentoPaginadoABuscar);
		int maxLinea = config_get_int_value(config, "MAX_LINEA");
		int posicionDato = numLineaACambiar * maxLinea;
		int paginaDelDato = posicionDato/config_get_int_value(config, "TAM_PAGINA");
		Pagina *pagina = list_get((*segmentoPaginadoBuscado).paginas,paginaDelDato);
		Marco *marco = list_get(lista_marcos,(*pagina).numeroMarco);
		memcpy(memoriaReal + (*marco).direccionBase,datoAescribir ,config_get_int_value(config, "MAX_LINEA")); //guardo el escriptorio en la memoria real
*/
		char * infoSegmentoPaginado = obtenerInfoSegmentoPaginado(paqueteDAM->archivoAsociado.numeroSegmento,paqueteDAM->idProcesoSolicitante);
		char ** lineas = string_split(infoSegmentoPaginado,"\n");
		int m = 0;
		char *nuevoCodigoArchivo = string_new();
		while(lineas[m] != NULL)
		{
			if(m == numLineaACambiar)
			{
				lineas[m] = string_new();
				string_append(&lineas[m],datoAescribir);
			}
			else
			{
				string_append(&nuevoCodigoArchivo,lineas[m]);
			}
			if(lineas[m+1] != NULL)
			{
				string_append(&nuevoCodigoArchivo,"\n");
			}
			m++;
		}
		log_info(logger,"Nuevo Codigo Archivo: %s", nuevoCodigoArchivo);
		t_config *config=config_create(pathFM9);
		int tamanioMaxPagina = config_get_int_value(config,"TAM_PAGINA");
		log_info(logger,"Paginando segmento...");
		int cantidadPaginasNecesarias = string_length(nuevoCodigoArchivo)/tamanioMaxPagina;
		int flag = 1;
		if((string_length(nuevoCodigoArchivo)%tamanioMaxPagina) != 0)
		{
			cantidadPaginasNecesarias++;
		}
		log_info(logger,"cantidadPaginas necesarias: %d ",cantidadPaginasNecesarias);
		for (int j = 0; j < cantActualMarcos; j++)
		{
			Pagina* unaPagina = list_get(segmentoPaginadoBuscado->paginas,j);
			Marco* unMarco = list_get(lista_marcos,unaPagina->numeroMarco);
			guardarInfoEnMemoria((*unMarco).direccionBase,nuevoCodigoArchivo,j);
		}
		if(cantidadPaginasNecesarias > cantActualMarcos) //hay que buscar los marcos que faltan
		{
			for (int i = cantActualMarcos; i < cantActualMarcos - cantidadPaginasNecesarias; i++)
			{
				 Pagina* nuevaPagina = malloc(sizeof(Pagina));
				 nuevaPagina->numeroPagina = i;
				 flag = agregarMarcoAtablaGeneral(nuevaPagina,nuevoCodigoArchivo,i);
				 if (flag == -1) //error , memoria insuficiente
				 {
					 return 20003;
				 }
				 list_add(segmentoPaginadoBuscado->paginas,nuevaPagina);
			}
			config_destroy(config);
		}
	}
	else if(!strcmp(modo,"TPI"))
	{

	}

	return 1 ;
}

void cambiarNumeroSegmento(Paquete_DAM* paqueteDAM,int numeroSegmentoOriginal)
{
	numSegmentoAbuscar = paqueteDAM->archivoAsociado.numeroSegmento;
	SegmentoDeTabla *segmentoBuscado =list_find(lista_segmentos,&segmentoEsIdABuscar);
	(*segmentoBuscado).numeroSegmento = numeroSegmentoOriginal;
}

char* obtenerInfoSegmento(int numeroSegmento,int idProcesoSolicitante)
{
	pthread_mutex_lock(&mutex_idTablaABuscar);
	idTablaBuscar = idProcesoSolicitante;
	TablaSegmentosDeProceso* unaTablaSegmentosDeProceso = list_find(lista_procesos,&esProcesoAbuscar);
	pthread_mutex_unlock(&mutex_idTablaABuscar);
	pthread_mutex_lock(&mutex_segmentoABuscar);
	numSegmentoAbuscar = numeroSegmento;
	SegmentoDeTabla *segmentoBuscado = list_find(lista_segmentos,&segmentoEsIdABuscar);
	pthread_mutex_unlock(&mutex_segmentoABuscar);
//	-----------------
//	char *infoArchivo = malloc((*segmentoBuscado).limite+1);
//	memcpy(infoArchivo,memoriaReal + (*segmentoBuscado).base,(*segmentoBuscado).limite);
	//--------
	int i = 0;
	char *aux = malloc(tamanioMaxLinea);
	char *infoArchivo = string_new();
	char *infoLineaArchivo = string_new();
	while (i < (*segmentoBuscado).limite/tamanioMaxLinea){
		memcpy(aux,memoriaReal + (*segmentoBuscado).base + i*tamanioMaxLinea,tamanioMaxLinea);
		log_info(logger,"aux: %s", aux);
		infoLineaArchivo = string_new();
		log_info(logger,"string_lenth(aux) =  %d", string_length(aux));
		log_info(logger,"aux = BarraN :  %d", strcmp(aux, "\n"));
		if(strcmp(aux, "\n") != 0)  //SI ES CERO ES PORQUE LA SIGUIENTE LINEA ES UN \n
		{
			string_append(&infoLineaArchivo,aux);
			char **partes = string_split(infoLineaArchivo,"\n");
			string_append(&infoArchivo,partes[0]);
			log_info(logger,"linea segmento: %s", partes[0]);
		}
		string_append(&infoArchivo,"\n");
		i++;
	}
	log_info(logger,"Info de Segmento obtenida");
	return infoArchivo;
}

char* obtenerInfoSegmentoPaginado(int numeroSegmento,int idProcesoSolicitante)
{
	/*
	char *infoArchivo = string_new();
	pthread_mutex_lock(&mutex_idTablaABuscar);
	idTablaBuscar = idProcesoSolicitante;
	TablaSegmentosDeProceso* unaTablaSegmentosDeProceso = list_find(lista_procesos,&esProcesoAbuscar);
	pthread_mutex_unlock(&mutex_idTablaABuscar);
	pthread_mutex_lock(&mutex_segmentoPaginadoABuscar);
	numSegmentoPaginadoAbuscar = numeroSegmento;
	SegmentoPaginadoDelProceso* segmentoPaginado = list_find(unaTablaSegmentosDeProceso->segmentosDelProceso,&segmentoPaginadoEsIdABuscar);
	pthread_mutex_unlock(&mutex_segmentoPaginadoABuscar);
	pthread_mutex_lock(&mutex_infoArchivoFlush);
	infoArchivoFlush = string_new();
	list_iterate(segmentoPaginado->paginas, &extraerInfoMarcoAsociado);
	string_append(&infoArchivo,infoArchivoFlush);
	pthread_mutex_unlock(&mutex_infoArchivoFlush);
	return infoArchivo;
	*/
	pthread_mutex_lock(&mutex_idTablaABuscar);
	idTablaBuscar = idProcesoSolicitante;
	TablaSegmentosDeProceso* unaTablaSegmentosDeProceso = list_find(lista_procesos,&esProcesoAbuscar);
	pthread_mutex_unlock(&mutex_idTablaABuscar);
	pthread_mutex_lock(&mutex_segmentoPaginadoABuscar);
	numSegmentoPaginadoAbuscar = numeroSegmento;
	SegmentoPaginadoDelProceso* segmentoPaginado = list_find(unaTablaSegmentosDeProceso->segmentosDelProceso,&segmentoPaginadoEsIdABuscar);
	log_info(logger,"Proceso..: %d",(*segmentoPaginado).idProceso);
	log_info(logger,"Proceso..(seg): %d",(*segmentoPaginado).numeroSegmento);
	log_info(logger,"Proceso..(listsizepag): %d",list_size(segmentoPaginado->paginas));
	pthread_mutex_unlock(&mutex_segmentoPaginadoABuscar);
	char *script = string_new();
	char* sentencia;
	for(int i = 0;i<list_size(segmentoPaginado->paginas);i++)
	{
		Pagina* paginaSegmento = list_get(segmentoPaginado->paginas,i);
		Marco* marco = list_get(lista_marcos,paginaSegmento->numeroMarco);
		log_info(logger,"Marco..: %d ",(*marco).numeroMarco);
		t_config *config2=config_create(pathFM9);
		int tamanioMaxPagina = config_get_int_value(config2,"TAM_PAGINA");
		sentencia = malloc(tamanioMaxPagina +1);
		memcpy(sentencia,memoriaReal + (*marco).direccionBase,tamanioMaxPagina);
		config_destroy(config2);
		sentencia[tamanioMaxPagina] = '\0';
		char ** lineas = string_split(sentencia,"\n");
		int m = 0;
		while(lineas[m] != NULL)
		{
			string_append(&script,lineas[m]);
			if(lineas[m+1] != NULL)
			{
				string_append(&script,"\n");
			}
			m++;
		}
		log_info(logger,"Sentencia: %s", script);
	}
	string_append(&script,"\0");  //puede que no vaya esto
	return script;
}

void extraerInfoMarcoAsociado(void * pagina){
	Pagina* paginaSegmento = (Pagina*) pagina;
	Marco* marco = list_get(lista_marcos,paginaSegmento->numeroMarco);
	t_config *config=config_create(pathFM9);
	int tamanioMaxPagina = config_get_int_value(config,"TAM_PAGINA");
	char *aux = string_new();
	memcpy(aux,memoriaReal + (*marco).direccionBase,tamanioMaxPagina);
	string_append(&infoArchivoFlush,aux); //vas concatenando los datos del archivo
	config_destroy(config);
}

int agregarSegmentoPaginado(char *nuevoEscriptorio,Paquete_DAM * paquete)
{
	log_info(logger,"Agregando segmento paginado...");
	int resultadoOperacion;
	int idProcesoSolicitante = paquete->idProcesoSolicitante;
	pthread_mutex_lock(&mutex_idTablaABuscar);
	idTablaBuscar = idProcesoSolicitante;
	TablaSegmentosDeProceso* unProceso = list_find(lista_procesos,&esProcesoAbuscar);
	pthread_mutex_unlock(&mutex_idTablaABuscar);
	if(unProceso == NULL)
	{
		log_info(logger,"Es un proceso nuevo...");
		TablaSegmentosDeProceso *procesoNuevo = malloc(sizeof(TablaSegmentosDeProceso));
		(*procesoNuevo).segmentosDelProceso = list_create();
		(*procesoNuevo).idProceso = idProcesoSolicitante;
		SegmentoPaginadoDelProceso *nuevoSegmentoPaginadoDelProceso = malloc(sizeof(SegmentoPaginadoDelProceso));
		(*nuevoSegmentoPaginadoDelProceso).numeroSegmento = idGlobal;
		(*nuevoSegmentoPaginadoDelProceso).paginas = list_create();
		idGlobal++;
		resultadoOperacion = paginarSegmento(nuevoSegmentoPaginadoDelProceso,nuevoEscriptorio);
		paquete->archivoAsociado.numeroSegmento = nuevoSegmentoPaginadoDelProceso->numeroSegmento;
		nuevoSegmentoPaginadoDelProceso->idProceso = paquete->idProcesoSolicitante ;
		if (resultadoOperacion != 20003)
		{
			list_add((*procesoNuevo).segmentosDelProceso,nuevoSegmentoPaginadoDelProceso);
			list_add(lista_procesos,procesoNuevo);
		}
	}
	else
	{
		log_info(logger,"Ya existia el proceso...");
		SegmentoPaginadoDelProceso *nuevoSegmentoPaginadoDelProceso2 = malloc(sizeof(SegmentoPaginadoDelProceso));
		(*nuevoSegmentoPaginadoDelProceso2).numeroSegmento = idGlobal;
		idGlobal++;
		(*nuevoSegmentoPaginadoDelProceso2).paginas = list_create();
		resultadoOperacion = paginarSegmento(nuevoSegmentoPaginadoDelProceso2,nuevoEscriptorio);
		paquete->archivoAsociado.numeroSegmento = nuevoSegmentoPaginadoDelProceso2->numeroSegmento;
		nuevoSegmentoPaginadoDelProceso2->idProceso = paquete->idProcesoSolicitante ;
		if (resultadoOperacion != 20003){
			list_add((*unProceso).segmentosDelProceso,nuevoSegmentoPaginadoDelProceso2);
		}
	}

	return resultadoOperacion;

}

int paginarSegmento(SegmentoPaginadoDelProceso *nuevoSegmentoPaginadoDelProceso,char *nuevoEscriptorio){
	t_config *config=config_create(pathFM9);
	int tamanioMaxPagina = config_get_int_value(config,"TAM_PAGINA");
	log_info(logger,"Paginando segmento...");
	int cantidadPaginas = string_length(nuevoEscriptorio)/tamanioMaxPagina;
	int flag = 1;
	if((string_length(nuevoEscriptorio)%tamanioMaxPagina) != 0)
	{
		cantidadPaginas++;
	}
	log_info(logger,"cantidadPaginas necesarias: %d ",cantidadPaginas);
	for (int i = 0; i < cantidadPaginas; i++)
	{
		 Pagina* nuevaPagina = malloc(sizeof(Pagina));
		 nuevaPagina->numeroPagina = i;
//		 nuevaPagina->numeroMarco = idMarcoGlobal;
		 flag = agregarMarcoAtablaGeneral(nuevaPagina,nuevoEscriptorio,i);
		 if (flag == -1) //error , memoria insuficiente
		 {
			 return 20003;
		 }
		 list_add(nuevoSegmentoPaginadoDelProceso->paginas,nuevaPagina);
	}
	config_destroy(config);
	return flag;
}

int agregarMarcoAtablaGeneral (Pagina* nuevaPagina,char *nuevoEscriptorio,int posicionEnArchivo)
{
	log_info(logger,"Agregando marco al proceso...");
	t_config *config=config_create(pathFM9);
	int tamanioMaxPagina = config_get_int_value(config,"TAM_PAGINA");
	pthread_mutex_lock(&mutex_ListaMarcos);
	Marco* unMarco = list_find(lista_marcos,&marcoEstaLibre);
	pthread_mutex_unlock(&mutex_ListaMarcos);
	if(unMarco == NULL)
	{
		if(idMarcoGlobal < cantidadMaximaMarcos)
		{
			log_info(logger,"Agregando nuevo marco a la memoria...");
			Marco* nuevoMarco = malloc(sizeof(Marco));
			(*nuevaPagina).numeroMarco = idMarcoGlobal;
			(*nuevoMarco).numeroMarco = (*nuevaPagina).numeroMarco;
			(*nuevoMarco).direccionBase = (*nuevoMarco).numeroMarco * tamanioMaxPagina;
			(*nuevoMarco).estaLibre = false;
			list_add(lista_marcos,nuevoMarco);
			idMarcoGlobal++;
			log_info(logger,"Marco - N°: %d ",(*nuevoMarco).numeroMarco);
			log_info(logger,"Marco - direccionBase: %d ",(*nuevoMarco).direccionBase);
			guardarInfoEnMemoria((*nuevoMarco).direccionBase,nuevoEscriptorio,posicionEnArchivo);
		}
		else
		{
			return -1 ; //no hay mas memoria;
		}
	}
	else
	{
		(*unMarco).estaLibre = false;
		(*nuevaPagina).numeroMarco = (*unMarco).numeroMarco;
		log_info(logger,"Marco - N°: %d ",(*unMarco).numeroMarco);
		log_info(logger,"Marco - direccionBase: %d ",(*unMarco).direccionBase);
		guardarInfoEnMemoria((*unMarco).direccionBase,nuevoEscriptorio,posicionEnArchivo);
	}
	config_destroy(config);
	return 1 ; //success
}

bool marcoEstaLibre(void *marcoDeTabla){
	Marco *b=(Marco*) marcoDeTabla;
	if((*b).estaLibre == true){
		return true;
	}
	else{
		return false;
	}
}

void guardarInfoEnMemoria(int direccionBaseMarco,char *nuevoEscriptorio, int posicionEnArchivo)
{
	log_info(logger,"Guardando info en memoria...");
	t_config *config=config_create(pathFM9);
	int tamanioMaxPagina = config_get_int_value(config,"TAM_PAGINA");
	memcpy(memoriaReal + direccionBaseMarco, nuevoEscriptorio + posicionEnArchivo*tamanioMaxPagina,tamanioMaxPagina); //guardo el escriptorio en la memoria real
	config_destroy(config);
	log_info(logger,"Contenido en memoria: %s", mem_hexstring(memoriaReal + direccionBaseMarco, tamanioMaxPagina));
}

void* consola()
{
	char** centinelas;
	log_info(logger, "Consola FM9");
	log_info(logger, "Ingrese el ID del DTB del cual quiere ver la informacion guardada en memoria:");
	char* linea = string_new();
	while(1) {
	    linea = readline(">");
	    if (!linea) {
	      break;
	    }
	//    int i =0;
	    centinelas = string_split(linea," ");
	    int n = cantidadDeCentinelas(centinelas);
	    log_info(logger, "Ieee :  %d ",n);
	    switch(n){
	    	case 1:
	    		log_info(logger, "ID DTB: %s ",centinelas[0]);
	    		t_config *config=config_create(pathFM9);
	    		char*modo= config_get_string_value(config, "MODO");
	    		if(!strcmp(modo,"SEG")){
	    				dumpSegmentacionPura(atoi(centinelas[0]));
	    			}
	    			else if(!strcmp(modo,"SPA")){
	    				dumpSegmentacionPaginada(atoi(centinelas[0]));
	    			}
	    		config_destroy(config);
	    		break;
	    	default:
	    		printf("Usted ingreso una cantidad de argumentos invalida\n");
	    		log_error(logger, "Cantidad de argumentos invalida");
	    }
	//    log_destroy(logger);
	    free(centinelas);
	    free(linea);
	  }

}

int cantidadDeCentinelas(char** centinelas){
	int i = 0;
	while(centinelas[i]){
		i++;
	}
	return i;
}

void dumpSegmentacionPura(int idDTB)
{
	pthread_mutex_lock(&mutex_idTablaABuscar);
	idTablaBuscar = idDTB;
	TablaSegmentosDeProceso* unaTablaSegmentosDeProceso = list_find(lista_procesos,&esProcesoAbuscar);
	pthread_mutex_unlock(&mutex_idTablaABuscar);
	if(unaTablaSegmentosDeProceso != NULL)
	{
		numArchivo = 1 ;
		log_info(logger, "El DTB con id %d, tiene %d archivos abiertos en memoria", idDTB, list_size(unaTablaSegmentosDeProceso->segmentosDelProceso));
		list_iterate(unaTablaSegmentosDeProceso->segmentosDelProceso, &mostrarInfoSegmento);
	}
	else
	{
		log_info(logger, "El DTB pedido no tiene datos almacenados en memoria");
	}
}

void mostrarInfoSegmento(SegmentoDelProceso* segmento)
{
	log_info(logger,"Datos Archivo N° %d",numArchivo);
	log_info(logger,"Contenido segmento \n: %s", mem_hexstring(memoriaReal + (*segmento).base, (*segmento).limite));
	numArchivo++;
}


void dumpSegmentacionPaginada(int idDTB)
{
	char *infoArchivo = string_new();
	pthread_mutex_lock(&mutex_idTablaABuscar);
	idTablaBuscar = idDTB;
	TablaSegmentosDeProceso* unaTablaSegmentosDeProceso = list_find(lista_procesos,&esProcesoAbuscar);
	pthread_mutex_unlock(&mutex_idTablaABuscar);
	if(unaTablaSegmentosDeProceso != NULL)
	{
		log_info(logger, "El DTB con id %d, tiene %d archivos abiertos en memoria", idDTB, list_size(unaTablaSegmentosDeProceso->segmentosDelProceso));
		numArchivo = 1 ;
		list_iterate(unaTablaSegmentosDeProceso->segmentosDelProceso, &mostrarInfoSegmentoPaginado);
	}
	else
	{
		log_info(logger, "El DTB pedido no tiene datos almacenados en memoria");
	}
}

void mostrarInfoSegmentoPaginado(SegmentoPaginadoDelProceso *segmentoPaginado)
{
//	log_info(logger,"Contenido segmento \n: %s", mem_hexstring(memoriaReal + (*segmento).base, (*segmento).limite));
	/*char *infoArchivo = string_new();
	pthread_mutex_lock(&mutex_infoArchivoFlush);
	infoArchivoFlush = string_new();
	list_iterate(segmentoPaginado->paginas, &extraerInfoMarcoAsociado);
	string_append(&infoArchivo,infoArchivoFlush);
	pthread_mutex_unlock(&mutex_infoArchivoFlush);*/
	log_info(logger,"Proceso..: %d",(*segmentoPaginado).idProceso);
	log_info(logger,"Proceso..(seg): %d",(*segmentoPaginado).numeroSegmento);
	log_info(logger,"Proceso..(listsizepag): %d",list_size(segmentoPaginado->paginas));
	char *script = string_new();
	char* sentencia;
	for(int i = 0;i<list_size(segmentoPaginado->paginas);i++)
	{
		Pagina* paginaSegmento = list_get(segmentoPaginado->paginas,i);
		Marco* marco = list_get(lista_marcos,paginaSegmento->numeroMarco);
		log_info(logger,"Marco..: %d ",(*marco).numeroMarco);
		t_config *config2=config_create(pathFM9);
		int tamanioMaxPagina = config_get_int_value(config2,"TAM_PAGINA");
		sentencia = malloc(tamanioMaxPagina +1);
		memcpy(sentencia,memoriaReal + (*marco).direccionBase,tamanioMaxPagina);
		config_destroy(config2);
		sentencia[tamanioMaxPagina] = '\0';
		char ** lineas = string_split(sentencia,"\n");
		int m = 0;
		while(lineas[m] != NULL)
		{
			string_append(&script,lineas[m]);
			if(lineas[m+1] != NULL)
			{
				string_append(&script,"\n");
			}
			m++;
		}
		log_info(logger,"Sentencia: %s", script);
	}
	string_append(&script,"\0");  //puede que no vaya esto
	log_info(logger, "El archivo N° %d tiene estos datos guardados: \n %s", numArchivo, script);
	numArchivo++;
}



