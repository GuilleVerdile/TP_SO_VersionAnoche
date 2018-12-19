#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include "FuncionesConexiones.h"
#include "Consola_S-AFA.h"
#include "S-AFA.h"

int guard(int n, char * err) { if (n == -1) { perror(err); exit(1); } return n; }
int cantidadDeCPUs = 0 ;
int cantidadProcesosEnMemoria = 0;
int sockDAM;
int quantumRestante;
int cpuInicial = 0;

void* conexionCPU(void* nuevoCliente){
	int sockCPU = *(int*)nuevoCliente; //asi estaba ..NO ANDA
//	ConexionCPU *cpu = list_get(cpu_conectadas,list_size(cpu_conectadas)-1);
//	int sockCPU = (*cpu).socketCPU;   //con esto lo copia bien el socket pero no anda el recv
//	int sockCPU = dup(sock);
	log_info(logger, "socket cpu dentro del hilo..: %d ",sockCPU);
//	send(sockCPU,'p',2,0); //prueba..dps borrar
	log_info(logger,"Estoy en el hilo del CPU");
	sem_post(&sem_esperaInicializacion);
	t_config *config=config_create(pathSAFA);
	int quantum = config_get_int_value(config, "Quantum");
	char *buf ;
	int instructionPointer ;
	while(1)
	{
		 buf = malloc(2);
//		 while(recv(sockCPU, buf, 2, 0) == -1){
//			 log_info(logger,"buffer por ahora.. : %s",buf);
//			 log_info(logger, "socket cpu dentro del hilo..: %d ",sockCPU);
//		 }
		 recv(sockCPU, buf, 2, 0);
		 log_info(logger,"llego un dato de cpu");
		 log_info(logger,"buffer.. : %s",buf);
		   //aca hago un case de los posibles send de una Cpu, que son
		   //f: termino de ejecutar; r: ejecuto una sentencia (puede desalojar por fin de quantum o no)
		   Estado estado;
		   int tam;
		   if((buf[0] == 'f') || (buf[0] == 'e'))
		   {	//se abortó su ejecucion
			   if (buf[0] == 'e')
			   {
				   log_info(logger,"Se aborta el DTBlock porque el script de la CPU tuvo que abortar debido a que ocurrio un error inesperado");
			   }
			   else //finalizo su ejecucion
			   {
				   log_info(logger,"La CPU termino de ejecutar su script");
			   }
			   pthread_mutex_lock(&mutex_proc_ejecucion);
			   pthread_mutex_lock(&mutex_socketProcEjecucionABuscar);
			   socketProcEjecucionABuscar = sockCPU;
			   Proceso *procesoEnExec = (Proceso*) list_find(procesosEnEjecucion,&procesoEnEjecucionEsIdABuscar);
			   list_remove_by_condition(procesosEnEjecucion,&procesoEnEjecucionEsIdABuscar);
			   pthread_mutex_unlock(&mutex_socketProcEjecucionABuscar);
			   if(procesoEnExec!=NULL){
				   idCPUBuscar = sockCPU;
				   log_info(logger,"EEEEE");
				   liberarRecursos(procesoEnExec);
				   ConexionCPU *cpuUtilizada = (ConexionCPU*) list_find(cpu_conectadas,&cpuUsadaEsIdABuscar);
//				   ConexionCPU *cpuUtilizada = list_get(cpu_conectadas,sockCPU);//fijarse si funciona
//				   log_info(logger,"conexion cpu es %d,el num %d", cpuUtilizada->socketCPU,cpuUtilizada->tieneDTBasociado );
				   (*cpuUtilizada).tieneDTBasociado = 0;
				   log_info(logger,"ANDA EL FIND");
				   list_add(procesos_terminados,procesoEnExec);
				   procesoEnExec->estado = finalizado ;
				   log_info(logger,"oooooooOooOooOOO");
//				   procesoEnEjecucion=NULL;
//				   sem_post(&sem_CPU_ejecutoUnaSentencia);
//				   if(list_size(procesos_listos) + list_size(procesos_bloqueados) < config_get_int_value(config, "Grado de multiprogramacion")) { //chequeo si puedo poner otro proceso en ready
//				  			sem_post(&sem_cargarProceso);    //desbloquea funcion que crea el dtbDummy
//				   }
//				   sem_post(&sem_replanificar);
				   log_info(logger,"aaaaaaaa");
				   pthread_mutex_lock(&mutex_pcp);
				   if(flag_replanificar < (list_size(cpu_conectadas) - list_size(procesosEnEjecucion))) //nadie mando a replanificar antes
				   {
						sem_post(&sem_replanificar);
						flag_replanificar++; //se esta replanificando y ocupando otra cpu
				   }
				   pthread_mutex_unlock(&mutex_pcp);
//				   quantumRestante = quantum; //reinicio el quantum para el proximo proceso que se vaya a ejecutar
			   }
			   pthread_mutex_unlock(&mutex_proc_ejecucion);
		   }
		   if(buf[0] == 'r')
		   { 	//CPU ejecuto una sentencia
			   log_info(logger,"La CPU ejecuto una sentencia");
			   analizarFinEjecucionLineaProceso(quantum,sockCPU);
		   }
		   if(buf[0] == 'd')  // DPS CAMBIAR ESTO PARA QUE  SE BLOQUEE EL DTBDUMMY (COMO ESTA ABAJO) Y CUANDO SE CARGA EL DTBDUMMY EN MEMORIA, SACARLO DE LA COLA DE BLOQUEADOS
		   { //CPU mando a ejecutar el dummy
			   log_info(logger,"La CPU mando  un dtbDummy al DAM");
			   pthread_mutex_lock(&mutex_socketProcEjecucionABuscar);
			   socketProcEjecucionABuscar = sockCPU;
			   Proceso *procesoEnExec = (Proceso*) list_find(procesosEnEjecucion,&procesoEnEjecucionEsIdABuscar);
			   list_remove_by_condition(procesosEnEjecucion,&procesoEnEjecucionEsIdABuscar);
			   pthread_mutex_unlock(&mutex_socketProcEjecucionABuscar);
			   pthread_mutex_lock(&mutex_proc_ejecucion);
			   procesoEnExec->estado = bloqueado;
			   pthread_mutex_lock(&mutex_cpuBuscar);
			   idCPUBuscar = (*procesoEnExec).socketProceso;
			   ConexionCPU *cpuUtilizada = (ConexionCPU*) list_find(cpu_conectadas,&cpuUsadaEsIdABuscar);
			   (*cpuUtilizada).tieneDTBasociado = 0;
			   pthread_mutex_unlock(&mutex_cpuBuscar);
			   log_info(logger,"Desligando CPU..");
//			   procesoEnEjecucion=NULL;
			   pthread_mutex_unlock(&mutex_proc_ejecucion);
//			   sem_post(&sem_replanificar);
			   pthread_mutex_lock(&mutex_pcp);
			   if(flag_replanificar < (list_size(cpu_conectadas) - list_size(procesosEnEjecucion))) //nadie mando a replanificar antes
			   {
					sem_post(&sem_replanificar);
					flag_replanificar++; //se esta replanificando y ocupando otra cpu
			   }
			   pthread_mutex_unlock(&mutex_pcp);
//			   quantumRestante = quantum;
		   }
		   if(buf[0] == 'b') //se debe bloquear el procesoEnEjecucion
		   {
			   pthread_mutex_lock(&mutex_socketProcEjecucionABuscar);
			   socketProcEjecucionABuscar = sockCPU;
			   Proceso *procesoEnExec = (Proceso*) list_find(procesosEnEjecucion,&procesoEnEjecucionEsIdABuscar);
			   list_remove_by_condition(procesosEnEjecucion,&procesoEnEjecucionEsIdABuscar);
			   pthread_mutex_unlock(&mutex_socketProcEjecucionABuscar);
			   procesoEnExec->quantumRestante--;
			   if(verificarSiFinalizarElProcesoEnEjecucion(quantum,sockCPU) == false) //si finalizo el proceso no lo tengo que bloquear
			   {
				   log_info(logger,"Se bloquea el DTBlock en ejecucion");
				   pthread_mutex_lock(&mutex_proc_ejecucion);
				   procesoEnExec->estado = bloqueado;
				   send((*procesoEnExec).socketProceso,"A",2,0);
				   recv((*procesoEnExec).socketProceso,&instructionPointer,sizeof(int),0);
				   (*procesoEnExec).seEstaCargando = instructionPointer;  //la linea de por donde va
				   pthread_mutex_lock(&mutex_cpuBuscar);
				   idCPUBuscar = (*procesoEnExec).socketProceso;
				   ConexionCPU *cpuUtilizada = (ConexionCPU*) list_find(cpu_conectadas,&cpuUsadaEsIdABuscar);
				   log_info(logger,"Desligando CPU..");
//				   usleep(200*1000);  //trampa no sirvio
				   (*cpuUtilizada).tieneDTBasociado = 0;
				   pthread_mutex_unlock(&mutex_cpuBuscar);
//				   log_info(logger,"Desligando CPU..");
				   ProcesoBloqueado *procesoAbloquear = malloc(sizeof(ProcesoBloqueado));
				   (*procesoAbloquear).proceso = procesoEnExec;
				   char*algoritmo= config_get_string_value(config, "Algoritmo");
				   if(!strcmp(algoritmo,"VRR")){
					   (*procesoAbloquear).quantumPrioridad = procesoEnExec->quantumRestante;
				   }
				   else
				   {
					   (*procesoAbloquear).quantumPrioridad = 0; //como no es VRR no hay cola mayor prioridad
				   }

				   list_add(procesos_bloqueados,procesoAbloquear);
//				   procesoEnEjecucion=NULL;
				   pthread_mutex_unlock(&mutex_proc_ejecucion);
//				   sem_post(&sem_replanificar);
				   pthread_mutex_lock(&mutex_pcp);
				   if(flag_replanificar < (list_size(cpu_conectadas) - list_size(procesosEnEjecucion))) //nadie mando a replanificar antes
				   {
						sem_post(&sem_replanificar);
						flag_replanificar++; //se esta replanificando y ocupando otra cpu
				   }
				   pthread_mutex_unlock(&mutex_pcp);
//				   quantumRestante = quantum;
			   }
//			   sem_post(&sem_CPU_ejecutoUnaSentencia);
		   }
		   if(buf[0] == 'w')
		   {
			   log_info(logger,"La CPU ejecuto una sentencia de wait");
			   char *buf2 ;
			   recv(sockCPU,&tam,sizeof(int),0);
			   buf2 = malloc(tam+1);
			   recv(sockCPU, buf2, tam, 0);  //PONER EN EL FINALIZAR DTBLOCK QUE SI AL FINALIZAR EL PROCESO DEBE LIBERAR LOS RECURSOS QUE RETENIA
			   //buf2 tiene el recurso ahora
			   buf2[tam]='\0';
			   pthread_mutex_lock(&mutex_recursoABuscar);
			   recursoABuscar = string_new();
			   string_append(&recursoABuscar,buf2);
			   Recurso *recurso = (Recurso*) list_find(recursos,&existeRecurso);
			   pthread_mutex_unlock(&mutex_recursoABuscar);
			   pthread_mutex_lock(&mutex_socketProcEjecucionABuscar);
			   socketProcEjecucionABuscar = sockCPU;
			   Proceso *procesoEnExec = (Proceso*) list_find(procesosEnEjecucion,&procesoEnEjecucionEsIdABuscar);
			   pthread_mutex_unlock(&mutex_socketProcEjecucionABuscar);
			   if(recurso)  //si existe el recurso
			   {
				   if((*recurso).instancias > 0)
				   {
					   (*recurso).instancias--;
					   pthread_mutex_lock(&mutex_recursoABuscar);
					   recursoABuscar = string_new();
					   string_append(&recursoABuscar,buf2);
					   Recurso *recursoYaRetenido = (Recurso*) list_find((*procesoEnExec).recursosRetenidos,&existeRecurso);
					   pthread_mutex_unlock(&mutex_recursoABuscar);
					   if(recursoYaRetenido){
						   (*recursoYaRetenido).instancias++;  //El proceso retiene otra instancia mas
					   }
					   else
					   {
						   Recurso *recursoRetenido = malloc(sizeof(Recurso));
						   (*recursoRetenido).nombre = (*recurso).nombre;
						   (*recursoRetenido).instancias = 1;
						   list_add((*procesoEnExec).recursosRetenidos,recursoRetenido);
					   }
					   //Analizo lo mismo que cuando ejecuto una sentencia comun (cpun envia 'r')
					   analizarFinEjecucionLineaProceso(quantum,sockCPU);
				   }
				   else
				   {
					   procesoEnExec->quantumRestante--;
					   if(verificarSiFinalizarElProcesoEnEjecucion(quantum,sockCPU) == false) //si finalizo el proceso no lo tengo que bloquear
					   {
						   (*recurso).instancias--;
						   log_info(logger,"Se bloquea el proceso por falta de instancias disponibles del recurso pedido");
						   list_add((*recurso).procesosEnEspera,procesoEnExec);
						   ProcesoBloqueado *procesoAbloquear = malloc(sizeof(ProcesoBloqueado));
						   (*procesoAbloquear).proceso = procesoEnExec;
						   (*procesoAbloquear).quantumPrioridad = 0;
						   list_add(procesos_bloqueados,procesoAbloquear);
						   pthread_mutex_lock(&mutex_socketProcEjecucionABuscar);
						   socketProcEjecucionABuscar = sockCPU;
						   list_remove_by_condition(procesosEnEjecucion,&procesoEnEjecucionEsIdABuscar);
						   pthread_mutex_unlock(&mutex_socketProcEjecucionABuscar);
						   log_info(logger,"Desligando CPU..");
						   send((*procesoEnExec).socketProceso,"A",2,0);
						   recv((*procesoEnExec).socketProceso,&instructionPointer,sizeof(int),0);
						   (*procesoEnExec).seEstaCargando = instructionPointer;  //por donde va ejecutando
						   pthread_mutex_lock(&mutex_cpuBuscar);
						   idCPUBuscar = (*procesoEnExec).socketProceso;
						   ConexionCPU *cpuUtilizada = (ConexionCPU*) list_find(cpu_conectadas,&cpuUsadaEsIdABuscar);
						   (*cpuUtilizada).tieneDTBasociado = 0;
						   pthread_mutex_unlock(&mutex_cpuBuscar);
						   pthread_mutex_lock(&mutex_proc_ejecucion);
						   procesoEnExec->estado = bloqueado ;
//						   procesoEnEjecucion=NULL;
						   pthread_mutex_unlock(&mutex_proc_ejecucion);
//						   sem_post(&sem_replanificar);
						   pthread_mutex_lock(&mutex_pcp);
						   if(flag_replanificar < (list_size(cpu_conectadas) - list_size(procesosEnEjecucion))) //nadie mando a replanificar antes
						   {
								sem_post(&sem_replanificar);
								flag_replanificar++; //se esta replanificando y ocupando otra cpu
						   }
						   pthread_mutex_unlock(&mutex_pcp);
//						   quantumRestante = quantum;
					   }
//					   sem_post(&sem_CPU_ejecutoUnaSentencia);
				   }
			   }
			   else //intancias = 0
			   {
				   log_info(logger,"Creando nuevo recurso");
				   Recurso *recursoNuevo = malloc(sizeof(Recurso));
				   (*recursoNuevo).nombre = string_new();
				   string_append(&(*recursoNuevo).nombre,buf2);
				   (*recursoNuevo).instancias = 1;	//lo inicializo en 1
				   (*recursoNuevo).instancias--;    //le decremento 1 instancia por el wait
				   (*recursoNuevo).procesosEnEspera = list_create();
				   Recurso *recursoNuevoARetener = malloc(sizeof(Recurso));
				   (*recursoNuevoARetener).nombre = (*recursoNuevo).nombre;
				   (*recursoNuevoARetener).instancias = 1;
				   list_add((*procesoEnExec).recursosRetenidos,recursoNuevoARetener);
				   list_add(recursos,recursoNuevo);
				   	printf("probando de nuevo.. ");
				   //Analizo lo mismo que cuando ejecuto una sentencia comun (cpun envia 'r')
				   analizarFinEjecucionLineaProceso(quantum,sockCPU);
			   }

			   free(buf2);
		   }
		   if(buf[0] == 's')
		   {
			   log_info(logger,"La CPU ejecuto una sentencia de signal");
			   char *buf3 ;
			   recv(sockCPU,&tam,sizeof(int),0);
			   buf3 = malloc(tam+1);
			   recv(sockCPU, buf3, tam, 0);   //buf3 tiene el recurso ahora
			   pthread_mutex_lock(&mutex_recursoABuscar);
			   recursoABuscar = string_new();
			   buf3[tam]='\0';
			   string_append(&recursoABuscar,buf3);
			   Recurso *recurso2 = (Recurso*) list_find(recursos,&existeRecurso);
			   pthread_mutex_unlock(&mutex_recursoABuscar);
			   if(recurso2)  //si existe el recurso
			   {
				   if((*recurso2).instancias >= 0){
					   (*recurso2).instancias++;
				   }
				   else  //intancias < 0
				   {
					   log_info(logger,"Se desbloquea el primer proceso que estaba esperando el recurso en cuestion");
					   (*recurso2).instancias++;
					   Proceso *procesoADesbloquear = (Proceso*) list_remove((*recurso2).procesosEnEspera,0);
					   pthread_mutex_lock(&mutex_idBuscar);
					   idBuscar = (*procesoADesbloquear).idProceso;
//					   procesoADesbloquear = (ProcesoBloqueado*) list_find(procesos_bloqueados,&procesoEsIdABuscar);
					   list_remove_by_condition(procesos_bloqueados,&procesoBloqueadoEsIdABuscar);
					   pthread_mutex_unlock(&mutex_idBuscar);
					   list_add(procesos_listos,procesoADesbloquear);
					   //poner aca el super if para ver si tiene que replanificar
				   }
			   }
			   else
			   {
				   log_info(logger,"Creando nuevo recurso");
				   Recurso *recursoNuevo2 = malloc(sizeof(Recurso));
				   (*recursoNuevo2).nombre = string_new();
				   string_append(&(*recursoNuevo2).nombre,buf3);
				   (*recursoNuevo2).instancias = 1;	//lo inicializo en 1
				   (*recursoNuevo2).procesosEnEspera = list_create();
				   list_add(recursos,recursoNuevo2);
			   }
			   //Analizo lo mismo que cuando ejecuto una sentencia comun (cpu envia 'r')
			   analizarFinEjecucionLineaProceso(quantum,sockCPU);
			   free(buf3);
		   }
		 free(buf);
	}
	return 0;
}


void* conexionDAM(void* nuevoCliente){
	int sockDAM = *(int*)nuevoCliente;
	sem_post(&sem_esperaInicializacion);
	log_info(logger,"Estoy en el hilo del DAM");
//	void* buf = malloc(sizeof(Paquete_DAM));
	int operacionRealizada;
	int cantCPUsLibre;
	ProcesoBloqueado *proceso;
//	while(1){
		Paquete_DAM *paquete= malloc (sizeof(Paquete_DAM));
		recibirPaqueteDAM(sockDAM,paquete);
		log_info(logger,"Se recibio un paquete de fin de operacion del DAM");
		operacionRealizada = (*paquete).tipoOperacion ;
		switch(operacionRealizada)
		{
			case 1:  //Abrir archivo
				pthread_mutex_lock(&mutex_idBuscar);
				idBuscar = (*paquete).idProcesoSolicitante;
				Proceso *procesoAcargar = (Proceso*) list_find(procesos_nuevos,&procesoEsIdABuscar);
				list_remove_by_condition(procesos_nuevos,&procesoEsIdABuscar);
				pthread_mutex_unlock(&mutex_idBuscar);
				log_info(logger,"Resultado operacion del DAM: %d ID %d", (*paquete).resultado,(*paquete).idProcesoSolicitante);
				if(procesoAcargar){ //significa que era un abrir de un DTBdummy
					if ((*paquete).resultado == 1){ //operacion abrir realizada con exito
						log_info(logger,"DAM pudo abrir el archivo");
						(*procesoAcargar).seEstaCargando = 0 ; //va a arrancar a ejecutar desde la primer linea del script..
						(*procesoAcargar).flagInicializacion = 1;
						DatosArchivo *archivoAsociado = malloc(sizeof(DatosArchivo));
						(*archivoAsociado).nombre = string_new();
						string_append(&((*archivoAsociado).nombre), (*paquete).archivoAsociado.nombre);
						(*archivoAsociado).direccionContenido = string_new();
						string_append(&((*archivoAsociado).direccionContenido), (*paquete).archivoAsociado.direccionContenido);
						(*archivoAsociado).numeroPagina = (*paquete).archivoAsociado.numeroPagina;
						(*archivoAsociado).numeroSegmento = (*paquete).archivoAsociado.numeroSegmento;
						list_add((*procesoAcargar).archivosAsociados,archivoAsociado);
						(*procesoAcargar).pathEscriptorio = string_new();
						string_append(&((*procesoAcargar).pathEscriptorio), (*paquete).archivoAsociado.direccionContenido);
//						strcpy(archivoAbuscar,(*paquete).archivoAsociado.nombre);
//						DatosArchivo *archivoAsociadoCargado = (DatosArchivo*) list_find((*procesoAcargar).archivosAsociados,&esArchivoBuscado);
						list_add(procesos_listos,procesoAcargar) ;
						log_info(logger,"Se agrego un proceso (luego de ejecutar dummy) a la cola de listos");
						log_info(logger,"cantidad procesos listos..: %d",list_size(procesos_listos));
						pthread_mutex_lock(&mutex_proc_ejecucion);
						int cantCPUsLibre = list_size(cpu_conectadas) - list_size(procesosEnEjecucion);
						if(cantCPUsLibre>0){
							pthread_mutex_lock(&mutex_pcp);
							if(flag_replanificar < (list_size(cpu_conectadas) - list_size(procesosEnEjecucion))) //nadie mando a replanificar antes
							{
								sem_post(&sem_replanificar);
								flag_replanificar++; //se esta replanificando y ocupando otra cpu
							}
							pthread_mutex_unlock(&mutex_pcp);
						}
						pthread_mutex_unlock(&mutex_proc_ejecucion);
					}
					if ((*paquete).resultado == 0){  //no se pudo abrir correctamente el archivo
						log_info(logger,"Operacion abrir no se pudo realizar correctamente.. enviando proceso a cola de exit");
						list_add(procesos_terminados,procesoAcargar) ;
					}
	//				procesoEnEjecucion->estado = finalizado ;
	//				procesoEnEjecucion=NULL;
	//				sem_post(&sem_CPU_ejecutoUnaSentencia);
	//				sem_post(&sem_replanificar);
				}
				else {  //era un abrir dentro del codigo escriptorio
					if ((*paquete).resultado == 1){ //operacion abrir realizada con exito
						log_info(logger,"abrir test");
						pthread_mutex_lock(&mutex_idBuscar);
						idBuscar = (*paquete).idProcesoSolicitante;
						ProcesoBloqueado *procesoADesbloquear = (ProcesoBloqueado*) list_find(procesos_bloqueados,&procesoBloqueadoEsIdABuscar);
						list_remove_by_condition(procesos_bloqueados,&procesoBloqueadoEsIdABuscar);
						pthread_mutex_unlock(&mutex_idBuscar);
						log_info(logger,"abrir test2");
						DatosArchivo *archivoAsociado = malloc(sizeof(DatosArchivo));
						(*archivoAsociado).nombre = string_new();
						string_append(&((*archivoAsociado).nombre), (*paquete).archivoAsociado.nombre);
						(*archivoAsociado).direccionContenido = string_new();
						string_append(&((*archivoAsociado).direccionContenido), (*paquete).archivoAsociado.direccionContenido);
						(*archivoAsociado).numeroPagina = (*paquete).archivoAsociado.numeroPagina;
						(*archivoAsociado).numeroSegmento = (*paquete).archivoAsociado.numeroSegmento;
						log_info(logger,"numSegmento.. %d",(*archivoAsociado).numeroSegmento);
						list_add(procesoADesbloquear->proceso->archivosAsociados,archivoAsociado);
						t_config *config=config_create(pathSAFA);
						char*algoritmo= config_get_string_value(config, "Algoritmo");
						log_info(logger,"abrir test3");
						if(!strcmp(algoritmo,"VRR")){
							list_add(procesos_listos_MayorPrioridad,procesoADesbloquear);  //CREO Q ACA DEBERIA IR procesoADesbloquear->proceso
						}
						else
						{
							list_add(procesos_listos,procesoADesbloquear->proceso);
						}
						config_destroy(config);
						pthread_mutex_lock(&mutex_proc_ejecucion);
						int cantCPUsLibre = list_size(cpu_conectadas) - list_size(procesosEnEjecucion);
						if(cantCPUsLibre>0){
							pthread_mutex_lock(&mutex_pcp);
							if(flag_replanificar < (list_size(cpu_conectadas) - list_size(procesosEnEjecucion))) //nadie mando a replanificar antes
							{
								sem_post(&sem_replanificar);
								flag_replanificar++; //se esta replanificando y ocupando otra cpu
							}
							pthread_mutex_unlock(&mutex_pcp);
						}
						pthread_mutex_unlock(&mutex_proc_ejecucion);
					}
					if ((*paquete).resultado == 0){  //no se pudo abrir correctamente el archivo (path inexistente en MDJ)
						log_info(logger,"Operacion abrir no se pudo realizar correctamente.. enviando proceso a cola de exit");
						pthread_mutex_lock(&mutex_idBuscar);
						idBuscar = (*paquete).idProcesoSolicitante;
						ProcesoBloqueado *procesoAFinalizar = (ProcesoBloqueado*) list_find(procesos_bloqueados,&procesoBloqueadoEsIdABuscar);
						list_remove_by_condition(procesos_bloqueados,&procesoBloqueadoEsIdABuscar);
						liberarRecursos(procesoAFinalizar->proceso);
						pthread_mutex_unlock(&mutex_idBuscar);
						list_add(procesos_terminados,procesoAFinalizar->proceso) ;
					}
					if ((*paquete).resultado == 10002){  //no se pudo abrir correctamente el archivo (espacion insuficiente de memoria en FM9)
						log_info(logger,"Se aborta el CPU por memoria insuficiente en FM9.");
						log_info(logger,"Mando orden a CPU para que avise a FM9 que libere la memoria utilizada por la ejecución del proceso.");
						pthread_mutex_lock(&mutex_idBuscar);
						idBuscar = (*paquete).idProcesoSolicitante;
						ProcesoBloqueado *procesoAFinalizar2 = (ProcesoBloqueado*) list_find(procesos_bloqueados,&procesoBloqueadoEsIdABuscar);
						list_remove_by_condition(procesos_bloqueados,&procesoBloqueadoEsIdABuscar);
						pthread_mutex_unlock(&mutex_idBuscar);
						send((*procesoAFinalizar2).proceso->socketProceso,"E",2,0);
						log_info(logger,"Operacion abrir no se pudo realizar correctamente.. enviando proceso a cola de exit");
						idCPUBuscar = (*procesoAFinalizar2).proceso->socketProceso;
					    liberarRecursos((*procesoAFinalizar2).proceso);
					    ConexionCPU *cpuUtilizada = (ConexionCPU*) list_find(cpu_conectadas,&cpuUsadaEsIdABuscar);
					    (*cpuUtilizada).tieneDTBasociado = 0;
					    log_info(logger,"Desligando CPU..");
					    procesoAFinalizar2->proceso->estado = finalizado ;
						list_add(procesos_terminados,(*procesoAFinalizar2).proceso) ;
					}
				}
			break;
			case 6:   //Porque es una operacion de flush
				pthread_mutex_lock(&mutex_idBuscar);
				idBuscar = (*paquete).idProcesoSolicitante;
				proceso = (ProcesoBloqueado*) list_find(procesos_bloqueados,&procesoBloqueadoEsIdABuscar);
				list_remove_by_condition(procesos_bloqueados,&procesoBloqueadoEsIdABuscar);
				pthread_mutex_unlock(&mutex_idBuscar);
				if((*paquete).resultado == 1){
					t_config *config=config_create(pathSAFA);
					char*algoritmo= config_get_string_value(config, "Algoritmo");
					if(!strcmp(algoritmo,"VRR")){
						list_add(procesos_listos_MayorPrioridad,proceso);
					}
					else
					{
						list_add(procesos_listos,proceso->proceso);
					}
					config_destroy(config);
				}
				if ((*paquete).resultado == 0){  //no se pudo flushear correctamente el archivo
					log_info(logger,"Operacion flush no se pudo realizar correctamente.. enviando proceso a cola de exit");
					liberarRecursos(proceso->proceso);
					list_add(procesos_terminados,proceso->proceso) ;
				}
				pthread_mutex_lock(&mutex_proc_ejecucion);
				cantCPUsLibre = list_size(cpu_conectadas) - list_size(procesosEnEjecucion);
				if(cantCPUsLibre>0){
					pthread_mutex_lock(&mutex_pcp);
					if(flag_replanificar < (list_size(cpu_conectadas) - list_size(procesosEnEjecucion))) //nadie mando a replanificar antes
					{
						sem_post(&sem_replanificar);
						flag_replanificar++; //se esta replanificando y ocupando otra cpu
					}
					pthread_mutex_unlock(&mutex_pcp);
				}
				pthread_mutex_unlock(&mutex_proc_ejecucion);
			break;
			case 8: //Porque es una operacion de crear
				pthread_mutex_lock(&mutex_idBuscar);
				idBuscar = (*paquete).idProcesoSolicitante;
				proceso = (ProcesoBloqueado*) list_find(procesos_bloqueados,&procesoBloqueadoEsIdABuscar);
				list_remove_by_condition(procesos_bloqueados,&procesoBloqueadoEsIdABuscar);
				pthread_mutex_unlock(&mutex_idBuscar);
				if((*paquete).resultado == 1){
					log_info(logger,"Me aviso DAM que el archivo se creo bien");
					t_config *config=config_create(pathSAFA);
					char*algoritmo= config_get_string_value(config, "Algoritmo");
					if(!strcmp(algoritmo,"VRR")){
						list_add(procesos_listos_MayorPrioridad,proceso);
					}
					else
					{
						list_add(procesos_listos,proceso->proceso);
					}
					config_destroy(config);
				}
				if ((*paquete).resultado == 0){  //no se pudo crear correctamente el archivo
					log_info(logger,"Operacion crear no se pudo realizar correctamente.. enviando proceso a cola de exit");
//					-----------
					log_info(logger,"Se aborta el CPU por memoria insuficiente en MDJ.");
					send((*proceso).proceso->socketProceso,"A",2,0);
					liberarRecursos((*proceso).proceso);
					pthread_mutex_lock(&mutex_cpuBuscar);
					idCPUBuscar = (*proceso).proceso->socketProceso;
					ConexionCPU *cpuUtilizada = (ConexionCPU*) list_find(cpu_conectadas,&cpuUsadaEsIdABuscar);
					(*cpuUtilizada).tieneDTBasociado = 0;
				    pthread_mutex_unlock(&mutex_cpuBuscar);
					log_info(logger,"Desligando CPU..");
					proceso->proceso->estado = finalizado ;
					list_add(procesos_terminados,proceso->proceso) ;
				}
				pthread_mutex_lock(&mutex_proc_ejecucion);
				log_info(logger," che cpuslibres: %d", list_size(cpu_conectadas) - list_size(procesosEnEjecucion));
				log_info(logger," a ver pibe flagrep: %d", flag_replanificar);
				cantCPUsLibre = list_size(cpu_conectadas) - list_size(procesosEnEjecucion);
				if(cantCPUsLibre>0){
					pthread_mutex_lock(&mutex_pcp);
					log_info(logger,"flagrep: %d", flag_replanificar);
					if(flag_replanificar < (list_size(cpu_conectadas) - list_size(procesosEnEjecucion))) //nadie mando a replanificar antes
					{
						sem_post(&sem_replanificar);
						flag_replanificar++; //se esta replanificando y ocupando otra cpu
					}
					pthread_mutex_unlock(&mutex_pcp);
				}
				pthread_mutex_unlock(&mutex_proc_ejecucion);
			break;
			case 9: //Porque es una operacion de borrar
				pthread_mutex_lock(&mutex_idBuscar);
				idBuscar = (*paquete).idProcesoSolicitante;
				proceso = (ProcesoBloqueado*) list_find(procesos_bloqueados,&procesoBloqueadoEsIdABuscar);
				list_remove_by_condition(procesos_bloqueados,&procesoBloqueadoEsIdABuscar);
				pthread_mutex_unlock(&mutex_idBuscar);
				if((*paquete).resultado == 1){
					t_config *config=config_create(pathSAFA);
					char*algoritmo= config_get_string_value(config, "Algoritmo");
					if(!strcmp(algoritmo,"VRR")){
						list_add(procesos_listos_MayorPrioridad,proceso);
					}
					else
					{
						list_add(procesos_listos,proceso->proceso);
					}
					config_destroy(config);
				}
				if ((*paquete).resultado == 0){  //no se pudo borrar correctamente el archivo
					log_info(logger,"Operacion borrar no se pudo realizar correctamente.. enviando proceso a cola de exit");
					liberarRecursos(proceso->proceso);
					list_add(procesos_terminados,proceso->proceso) ;
				}
				pthread_mutex_lock(&mutex_proc_ejecucion);
				cantCPUsLibre = list_size(cpu_conectadas) - list_size(procesosEnEjecucion);
				if(cantCPUsLibre>0){
					pthread_mutex_lock(&mutex_pcp);
					if(flag_replanificar < (list_size(cpu_conectadas) - list_size(procesosEnEjecucion))) //nadie mando a replanificar antes
					{
						sem_post(&sem_replanificar);
						flag_replanificar++; //se esta replanificando y ocupando otra cpu
					}
					pthread_mutex_unlock(&mutex_pcp);
				}
				pthread_mutex_unlock(&mutex_proc_ejecucion);
			break;
		}

//	}
	close(sockDAM);
	return 0;
}


int atenderConexiones() {
	int sockSAFA;
	sockSAFA=crearConexionServidor(pathSAFA);
	if (listen(sockSAFA, 10) == -1){ //ESCUCHA!
		    log_error(logger, "No se pudo escuchar");
			log_destroy(logger);
			return -1;
	}
	log_info(logger, "Se esta escuchando");
  int i=0;
  char* buff;
  int cantidadPedidosDAM = 0;
	pthread_t hiloCPU;
	pthread_t* hilosPedidosDAM = NULL;
	pthread_t hiloDAM;
	pthread_t* hilosCPUs = NULL;
	int tipoCliente,sockCliente;
	int flag1 = 0;
  while((sockCliente = accept(sockSAFA, NULL, NULL))) {
		if (sockCliente == -1) {
			 log_error(logger,"error when accepting connection");
			return -1;
		} else {
		  i++;
		  buff = malloc(2);
		  recv(sockCliente,buff,2,0);
		  log_info(logger, "Llego una nueva conexion");
		  if(buff[0] == '1'){
			  log_info(logger, "socket cpu: %d ",sockCliente); //dps borrar
			  ConexionCPU *nuevaCPU = malloc(sizeof(ConexionCPU));
			  (*nuevaCPU).tieneDTBasociado=0;
			  (*nuevaCPU).socketCPU=sockCliente;
//			  list_add_in_index(cpu_conectadas, sockCliente, nuevaCPU);//probar si funciona
		      list_add(cpu_conectadas,nuevaCPU);
//			  sockCPU = sockCliente; //dps borrar
			  hilosCPUs = realloc(hilosCPUs,sizeof(pthread_t)*(cantidadDeCPUs+1));
			  pthread_create(&(hilosCPUs[cantidadDeCPUs]),NULL,conexionCPU,(void*)&sockCliente);
			  cantidadDeCPUs++;
			  if(flag1 == 1){
				  sem_post(&sem_estadoOperativo);
			  }
		  }
		  else if(buff[0] == '2'){
//			  pthread_create(&hiloDAM,NULL,conexionDAM,(void*)&sockCliente);
//			  sockDAM = sockCliente;
			  log_info(logger,"Llego una nueva peticion del DAM");
			  hilosPedidosDAM = realloc(hilosPedidosDAM,sizeof(pthread_t)*(cantidadPedidosDAM+1));
			  pthread_create(&(hilosPedidosDAM[cantidadPedidosDAM]),NULL,conexionDAM,(void*)&sockCliente);
			  cantidadPedidosDAM++;
			  flag1 = 1;
			  if(cantidadDeCPUs >= 1){
				  sem_post(&sem_estadoOperativo);
			  }
		  }
		  else if(buff[0] == '5'){
			  log_info(logger,"Se conecto el DAM");
			  flag1 = 1;
			  sem_post(&sem_esperaInicializacion);
			  if(cantidadDeCPUs >= 1){
				  sem_post(&sem_estadoOperativo);
			  }
		  }
		  free(buff);
//      send(sockCliente, msg, sizeof(msg), 0);
//      close(sockCliente);
    }
		sem_wait(&sem_esperaInicializacion);
//		log_info(logger,"a ver pibe estoy esperando una nueva conexion...");
  }
  sleep(1);
  return 1;
}

void* inicializarSAFA(){
	t_config *config=config_create(pathSAFA);
	sem_wait(&sem_estadoOperativo);
	log_info(logger,"SAFA ahora esta en estado operativo");
	flag_seEnvioSignalPlanificar=0;
	flag_quierenDesalojar=0;
	flag_replanificar = 0;
	procesos_nuevos=list_create();
	procesos_listos=list_create();
	procesos_terminados=list_create();
	procesos_bloqueados=list_create();
	procesosEnEjecucion=list_create();
	recursos = list_create();
	sem_init(&sem_liberarRecursos,0,1);
	sem_init(&sem_liberador,0,0);
	sem_init(&sem_replanificar,0,0);
	sem_init(&sem_procesoEnEjecucion,0,0);
	sem_init(&sem_esperaInicializacion,0,0);
	sem_init(&sem_CPU_ejecutoUnaSentencia,0,0);
	sem_init(&sem_finDeEjecucion,0,1);
	sem_init(&semCambioEstado,0,0);
	sem_init(&sem_pausar,0,1);
	sem_init(&sem_cargarProceso,0,0);
	//
	pthread_mutex_init(&mutex_pausa,NULL);
	pthread_mutex_init(&mutex_proc_ejecucion,NULL);
	pthread_mutex_init(&mutex_socketProcEjecucionABuscar,NULL);
	pthread_mutex_init(&mutex_cpuBuscar,NULL);
	pthread_mutex_init(&mutex_pcp,NULL);
	pthread_mutex_init(&mutex_idBuscar,NULL);
	pthread_mutex_init(&mutex_recursoABuscar,NULL);
	idGlobal=1;
	bloquearDT = 0;
	pthread_t hilo_planificadorCortoPlazo;
	pthread_t liberadorRecursos;
	pthread_t hilo_ejecutarEsi;
	pthread_t hilo_consola;
	procesoEnEjecucion = NULL;
	Proceso*(*miAlgoritmo)();
	char*algoritmo= config_get_string_value(config, "Algoritmo");
	if(!strcmp(algoritmo,"FIFO")){
			miAlgoritmo=&fifo;
		flag_desalojo=0;
	}
	if(!strcmp(algoritmo,"RR")){
			miAlgoritmo=&round_robin;
//			quantumRestante = config_get_int_value(config, "Quantum");
	}
	if(!strcmp(algoritmo,"VRR")){
			procesos_listos_MayorPrioridad = list_create();
			miAlgoritmo=&virtual_round_robin;
	}
	log_info(logger,"Se asigno el algoritmo %s",algoritmo);
	pthread_create(&hilo_planificadorCortoPlazo,NULL,planificadorCortoPlazo,NULL);
	log_info(logger,"Se creo el hilo planificador CORTO PLAZO");
	pthread_create(&hilo_consola,NULL,(void *)consola,NULL);
	log_info(logger,"Se creo hilo para la CONSOLA");
	config_destroy(config);
	return 0;
}

int main(int argc, char**argv)
{
	logger =log_create(logSAFA,"S-AFA",1, LOG_LEVEL_INFO);
	if(argc > 1){
	    	pathSAFA = argv[1];
	}
	cpu_conectadas = list_create();
	sem_init(&sem_estadoOperativo,0,0);
	pthread_t hilo_inicializaciones;
	pthread_create(&hilo_inicializaciones,NULL,(void *)inicializarSAFA,NULL);
	atenderConexiones();
	//cerrarPlanificador();
	return 0;
}

Proceso* fifo(){
	Proceso *proceso=list_get(procesos_listos,0);
	list_remove(procesos_listos,0);
	return proceso;
}

Proceso* round_robin(){
	if(!list_is_empty(procesos_listos))
	{
		Proceso *proceso=list_get(procesos_listos,0);
		list_remove(procesos_listos,0);
		printf("probando");
		t_config *config=config_create(pathSAFA);
		(*proceso).quantumRestante = config_get_int_value(config, "Quantum");
		config_destroy(config);
		return proceso;
	}
	else
	{
		return NULL;
	}
}

Proceso* virtual_round_robin(){
	ProcesoBloqueado *procesoMayorPrioridad;
	Proceso* proceso;
	if(list_is_empty(procesos_listos_MayorPrioridad))
	{
		if(!list_is_empty(procesos_listos))
		{
			proceso=list_get(procesos_listos,0);
			list_remove(procesos_listos,0);
			t_config *config=config_create(pathSAFA);
	//		quantumRestante = config_get_int_value(config, "Quantum");
			(*proceso).quantumRestante = config_get_int_value(config, "Quantum");
			config_destroy(config);
		}
		else
		{
			return NULL;
		}
	}
	else
	{
		procesoMayorPrioridad=list_get(procesos_listos_MayorPrioridad,0);
		list_remove(procesos_listos_MayorPrioridad,0);
//		quantumRestante = procesoMayorPrioridad->quantumPrioridad;
		proceso = procesoMayorPrioridad->proceso;
		(*proceso).quantumRestante = procesoMayorPrioridad->quantumPrioridad;
	}

	return proceso;
}

void* planificadorLargoPlazo(void* pathCPU){
	char *pathEscriptorioCPU = (char *)pathCPU;
	Proceso* proceso= malloc(sizeof(Proceso));
	(*proceso).idProceso =idGlobal;
	(*proceso).estado=nuevo;
	(*proceso).flagInicializacion = 1;
	(*proceso).seEstaCargando = 0;
	(*proceso).pathEscriptorio = string_new();
	string_append(&((*proceso).pathEscriptorio), pathEscriptorioCPU);
	(*proceso).archivosAsociados = list_create();  //antes decia = NULL
	(*proceso).recursosRetenidos = list_create();
	log_info(logger, "Se asigno un nuevo dtb");
	//me fijo que tiene
	log_info(logger, "el path tienee %s con tam %d",(*proceso).pathEscriptorio, string_length((*proceso).pathEscriptorio) );//sacar
//	log_info(logger, "el tamaño de archivos asociados: %d ",list_size((*proceso).archivosAsociados) );//sacar
	list_add(procesos_nuevos, proceso);
	flag_nuevoProcesoEnListo = 1;
	idGlobal++;
//	sem_post(&sem_CPU_ejecutoUnaSentencia);  //ESTE NO VA
	t_config *config=config_create(pathSAFA);
//	if(list_size(procesos_listos) + list_size(procesos_bloqueados) + hayProcesoEnExec < config_get_int_value(config, "Multiprogramacion")) {
	log_info(logger,"Se dio senial para que arranque a crear el dtbdummy");
//		sem_post(&sem_cargarProceso);    //desbloquea funcion que crea el dtbDummy
	//	list_add(procesos_listos,proceso);
	cargarProcesoEnMemoria();
//		}
	config_destroy(config);
}
//	free (*proceso);


//void *cargarProcesoEnMemoria(){
//	while(1)
//	{
//	do
//	{
void cargarProcesoEnMemoria(){
//		sem_wait(&sem_cargarProceso);
		int pos = 0;
		Proceso *procesoAcargar = (Proceso*) list_get(procesos_nuevos,pos);    //PLP planifica segun FIFO
		while((*procesoAcargar).seEstaCargando == 1){ 	//pero me tengo que fijar de no agarrar a un dtb que se este inicializando con su dummy
			log_info(logger, "eeeeeee se trabo");
			pos++;
			procesoAcargar = (Proceso*) list_get(procesos_nuevos,pos);//cuando lo elimina de procesos_nuevos? sigue en linea 608
		}
		pos = 0;    //reinicio variables para proxima busqueda
		//Proceso *procesoAcargar = (Proceso*) list_get(procesos_nuevos,pos);
		(*procesoAcargar).seEstaCargando = 1 ;

//		-----------------------------------------------------
//		creo que deberiamos hacer un remove de lista proc nuevos y luego Proceso *procesoDummy =procesoAcargar
//		-----------------------------------------------------

		Proceso *procesoDummy=malloc(sizeof(Proceso));//pq malloc? que pasa con *proceso
		(*procesoDummy).idProceso = (*procesoAcargar).idProceso ;
		(*procesoDummy).socketProceso = (*procesoAcargar).socketProceso;
		(*procesoDummy).estado=listo;
		(*procesoDummy).flagInicializacion = 0;  //indicador que es un DtbDummy
		(*procesoDummy).pathEscriptorio = string_new();
		string_append(&((*procesoDummy).pathEscriptorio), (*procesoAcargar).pathEscriptorio);
		procesoDummy->archivosAsociados = list_create();
		procesoDummy->recursosRetenidos = list_create();
//		(*procesoDummy).pathEscriptorio = (*procesoAcargar).pathEscriptorio;
		log_info(logger, "Se asigno un nuevo dtbDummy");
//		-----------------------------------------------------
//      y entonces aca el proceso agregado a la lista procesos_listos es el  proceso original y no una copia
//		-----------------------------------------------------
		list_add(procesos_listos,procesoDummy);  //ahora espero a que el PCP planifique al DTBdummy...
		pthread_mutex_lock(&mutex_proc_ejecucion);
		int cantCPUsLibre = list_size(cpu_conectadas) - list_size(procesosEnEjecucion);
		if(cantCPUsLibre>0){
			pthread_mutex_lock(&mutex_pcp);
			if(flag_replanificar < (list_size(cpu_conectadas) - list_size(procesosEnEjecucion))) //nadie mando a replanificar antes
			{
				sem_post(&sem_replanificar);
				flag_replanificar++; //se esta replanificando y ocupando otra cpu
			}
			pthread_mutex_unlock(&mutex_pcp);
		}
		pthread_mutex_unlock(&mutex_proc_ejecucion);
//	}
//	while(1);
}

bool procesoEsIdABuscar(void * proceso){
	Proceso *proc=(Proceso*) proceso;
	if((*proc).idProceso==idBuscar)

		return true;
	else
		return false;
}
bool procesoBloqueadoEsIdABuscar(void * proceso){
	ProcesoBloqueado *proc=(ProcesoBloqueado*) proceso;
	if((*proc).proceso->idProceso==idBuscar)
		return true;
	else
		return false;
}

bool cpuUsadaEsIdABuscar(void * cpu){
	ConexionCPU *cpuConectada=(ConexionCPU*) cpu;
	if((*cpuConectada).socketCPU==idCPUBuscar)
		return true;
	else
		return false;
}


void *planificadorCortoPlazo(){//como parametro le tengo que pasar la direccion de memoria de mi funcion algoritmo
	// se tiene que ejecutar todo el tiempo en un hilo aparte
	t_config *config=config_create(pathSAFA);
	Proceso*(*miAlgoritmo)();
	char*algoritmo= config_get_string_value(config, "Algoritmo");
	if(!strcmp(algoritmo,"FIFO")){
			miAlgoritmo=&fifo;
		flag_desalojo=0;
	}
	if(!strcmp(algoritmo,"RR")){
			miAlgoritmo=&round_robin;
//			quantumRestante = config_get_int_value(config, "Quantum");
	}
	if(!strcmp(algoritmo,"VRR")){
			procesos_listos_MayorPrioridad = list_create();
			miAlgoritmo=&virtual_round_robin;
	}
	log_info(logger,"Se asigno el algoritmo %s",algoritmo);
	config_destroy(config);
	while(1)
	{
//		while(list_size(procesos_listos) == 0){
			//espera
//		}
		sem_wait(&sem_replanificar);
		log_info(logger,"se obtuvo senial de sem planificador");
		ejecutarRetardo(pathSAFA); //Se ejecuta el retardo antes de planificacion
		Proceso*(*algoritmo)();
		algoritmo=(Proceso*(*)()) miAlgoritmo;
		// aca necesito sincronizar para que se ejecute solo cuando le den la segnal de replanificar
		//
		Proceso *proceso; // este es el proceso que va a pasar de la cola de ready a ejecucion
		proceso = (*algoritmo)();//habia que inicializar las variables en dtbdummy en que momento sale de lista_nuevo?

		if(proceso)
		{
			log_info(logger,"PCP selecciono el Proceso ID %d",(*proceso).idProceso);
			ConexionCPU *unaCPU = malloc (sizeof(ConexionCPU));
			if((*proceso).flagInicializacion == 0){
				unaCPU = list_get(cpu_conectadas,cpuInicial);
				cpuInicial++;
				if(cpuInicial == list_size(cpu_conectadas))
				{
					cpuInicial=0 ;
				}
			}
			else
			{
				unaCPU = list_find(cpu_conectadas,&noTieneDTBasociado);
			}
			(*unaCPU).tieneDTBasociado = 1;
			(*proceso).socketProceso = (*unaCPU).socketCPU;
			log_info(logger, "Se reservo una CPU para el DTB");
			(*proceso).estado=ejecucion;
			pthread_mutex_lock(&mutex_proc_ejecucion);
			list_add(procesosEnEjecucion,proceso);
			send((*proceso).socketProceso,"P",2,0);  //PROBANDO
			enviarProceso((*proceso).socketProceso, proceso);//mando el DTB al CPU asociado
			pthread_mutex_unlock(&mutex_proc_ejecucion);
			pthread_mutex_lock(&mutex_idBuscar);
			idBuscar = (*proceso).idProceso;
			list_remove_by_condition(procesos_listos,&procesoEsIdABuscar);
			pthread_mutex_unlock(&mutex_idBuscar);
//			void* buff = serializarProceso(procesoEnEjecucion);
//			send((*procesoEnEjecucion).socketProceso,buff,sizeof(Proceso),0);
			log_info(logger,"Se asigno el proceso como proceso en ejecucion");
			flag_seEnvioSignalPlanificar=0;
			pthread_mutex_lock(&mutex_proc_ejecucion);
			if((*proceso).flagInicializacion == 1){
//				sem_post(&sem_procesoEnEjecucion);  //para que habilite al hilo que ejecuta el proceso a seguir
				send((*proceso).socketProceso,"1",2,0);// este habilita a la cpu para que ejecute 1 sentencia de su codigo escriptorio
				log_info(logger,"Se da orden  a la CPU de ejecutar el proceso: %d",(*proceso).idProceso);
			}
			pthread_mutex_unlock(&mutex_proc_ejecucion);
		}
		pthread_mutex_lock(&mutex_pcp);
		flag_replanificar--;
		pthread_mutex_unlock(&mutex_pcp);
	}
}


void *ejecutarProceso(){
	while(1){
		sem_wait(&sem_procesoEnEjecucion);
		log_info(logger,"se entro a ejecutar el proceso en ejecucion");
		while(procesoEnEjecucion && (*procesoEnEjecucion).estado==ejecucion){
//			sem_wait(&sem_liberarRecursos);
			pthread_mutex_lock(&mutex_pausa);
			log_info(logger,"esperando semaforo de que la CPU ejecuto una sentencia");
			sem_wait(&sem_CPU_ejecutoUnaSentencia);
			if(procesoEnEjecucion){
				send((*procesoEnEjecucion).socketProceso,"1",2,0);// este habilita a la cpu para que ejecute 1 sentencia de su codigo escriptorio
				log_info(logger,"Se da orden  a la CPU de ejecutar el proceso: %d",(*procesoEnEjecucion).idProceso);
//				sem_post(&sem_liberarRecursos);
			}//EJECUCION
//			sem_wait(&semCambioEstado);
			pthread_mutex_unlock(&mutex_pausa);
		}
		//el proceso esi actual dejo de ser el que tiene que ejecutar
//		sem_post(&sem_finDeEjecucion);
		log_info(logger,"Se termino de ejecutar el CPU en cuestion");
	}

}

void finalizarProceso(char *idDT){
	int flag = 0;
    pthread_mutex_lock(&mutex_idBuscar);
    idBuscar = atoi(idDT);
    Proceso *procesoEnExec = (Proceso*) list_find(procesosEnEjecucion,&procesoEsIdABuscar);
    list_remove_by_condition(procesosEnEjecucion,&procesoEnEjecucionEsIdABuscar);
    pthread_mutex_unlock(&mutex_idBuscar);
	pthread_mutex_lock(&mutex_proc_ejecucion);
	if ((procesoEnExec != NULL) && (procesoEnExec->idProceso == atoi(idDT))){
		bloquearDT = 1;
		flag = 2;
	}
	else
	{
		pthread_mutex_lock(&mutex_idBuscar);
		idBuscar = atoi(idDT);
		Proceso *procesoAfinalizar = (Proceso*) list_find(procesos_nuevos,&procesoEsIdABuscar);
		if (procesoAfinalizar)
		{
			list_add(procesos_terminados,procesoAfinalizar);
			list_remove_by_condition(procesos_nuevos,&procesoEsIdABuscar);
			flag = 1;
		}
		Proceso *procesoAfinalizar2 = (Proceso*) list_find(procesos_listos,&procesoEsIdABuscar);
		if (procesoAfinalizar2)
		{
			liberarRecursos(procesoAfinalizar2);
			list_add(procesos_terminados,procesoAfinalizar2);
			list_remove_by_condition(procesos_listos,&procesoEsIdABuscar);
			flag = 1;
		}
		Proceso *procesoAfinalizar3 = (Proceso*) list_find(procesos_bloqueados,&procesoEsIdABuscar);
		if (procesoAfinalizar2)
		{
			liberarRecursos(procesoAfinalizar2);
			list_add(procesos_terminados,procesoAfinalizar3);
			list_remove_by_condition(procesos_bloqueados,&procesoEsIdABuscar);
			flag = 1;
		}
		pthread_mutex_unlock(&mutex_idBuscar);
		if (flag == 0)
		{
			log_info(logger,"El IDBlock ingresado no existe en el sistema");
		}
		else if (flag == 1)
		{
			log_info(logger,"El IDBlock ingresado fue finalizado exitosamente");
		}
	}
	pthread_mutex_unlock(&mutex_proc_ejecucion);
}

void statusProceso(char *idDT){
	int flag = 0;
	pthread_mutex_lock(&mutex_idBuscar);
	idBuscar = atoi(idDT);
    Proceso *procesoEnExec = (Proceso*) list_find(procesosEnEjecucion,&procesoEsIdABuscar);
    if(procesoEnExec)
    {
    	mostrarDatosDTBlock(procesoEnExec);
    	flag = 1;
    }
	Proceso *procesoStatus = (Proceso*) list_find(procesos_nuevos,&procesoEsIdABuscar);
	if (procesoStatus)
	{
		mostrarDatosDTBlock(procesoStatus);
		flag = 1;
	}
	Proceso *procesoStatus2 = (Proceso*) list_find(procesos_listos,&procesoEsIdABuscar);
	if (procesoStatus2)
	{
		mostrarDatosDTBlock(procesoStatus2);
		flag = 1;
	}
	Proceso *procesoStatus3 = (Proceso*) list_find(procesos_bloqueados,&procesoEsIdABuscar);
	if (procesoStatus3)
	{
		mostrarDatosDTBlock(procesoStatus3);
		flag = 1;
	}
	Proceso *procesoStatus4 = (Proceso*) list_find(procesos_terminados,&procesoEsIdABuscar);
	if (procesoStatus4)
	{
		mostrarDatosDTBlock(procesoStatus4);
		flag = 1;
	}
	if (flag == 0)
	{
		log_info(logger,"El IDBlock ingresado no existe en el sistema");
	}
	pthread_mutex_unlock(&mutex_idBuscar);

}

void mostrarDatosDTBlock(Proceso *procesoStatus)
{
	log_info(logger,"Datos DTBlock:");
	log_info(logger,"ID: %d",procesoStatus->idProceso);
	log_info(logger,"Path Escriptorio: %s",procesoStatus->pathEscriptorio);
	log_info(logger,"Socket CPU asociado: %d",procesoStatus->socketProceso);
	log_info(logger,"Flag Inicializacion: %d",procesoStatus->flagInicializacion);
	switch(procesoStatus->estado){
		case ejecucion:
		log_info(logger,"Estado: En Ejecucion");
		break;
		case nuevo:
		log_info(logger,"Estado: Nuevo");
		break;
		case bloqueado:
		log_info(logger,"Estado: Bloqueado");
		break;
		case listo:
		log_info(logger,"Estado: Listo");
		break;
		case finalizado:
		log_info(logger,"Estado: Finalizado");
		break;
	}
	log_info(logger,"Cantidad Archivos Abiertos: %d",list_size(procesoStatus->archivosAsociados));
	if(list_size(procesoStatus->archivosAsociados) > 0){
		 list_iterate(procesoStatus->archivosAsociados, &mostrarInfoArchivoAsociado);
	}
	log_info(logger,"Cantidad de Recursos Retenidos diferentes: %d",list_size(procesoStatus->recursosRetenidos));
	if(list_size(procesoStatus->recursosRetenidos) > 0){
		 list_iterate(procesoStatus->recursosRetenidos, &mostrarInfoRecursoRetenido);
	}



}

void mostrarInfoArchivoAsociado(DatosArchivo * archivoAsociado){
	log_info(logger,"Archivo Abierto:");
	log_info(logger,"Nombre: %s",archivoAsociado->nombre);
//	log_info(logger,"Direccion en memoria: %s",archivoAsociado->direccionContenido); //esto no se si hay q mostrarlo
}

void mostrarInfoRecursoRetenido(Recurso *recurso){
	log_info(logger,"Recurso Retenido:");
	log_info(logger,"Nombre: %s",recurso->nombre);
	log_info(logger,"Cantidad Instancias: %d",recurso->instancias);
}

void statusColasSistema(){
	log_info(logger,"Datos de Cada Cola del Sistema:");
	log_info(logger,"Cola procesos nuevos:");
	log_info(logger,"Cantidad DTBlocks en la cola: %d",list_size(procesos_nuevos));
	list_iterate(procesos_nuevos, &mostrarDatosDTBlock);
	log_info(logger,"Cola procesos listos:");
	log_info(logger,"Cantidad DTBlocks en la cola: %d",list_size(procesos_listos));
	list_iterate(procesos_listos, &mostrarDatosDTBlock);
	log_info(logger,"Cola procesos bloqueados:");
	log_info(logger,"Cantidad DTBlocks en la cola: %d",list_size(procesos_bloqueados));
	list_iterate(procesos_bloqueados, &mostrarDatosDTBlock);
	log_info(logger,"Cola procesos terminados:");
	log_info(logger,"Cantidad DTBlocks en la cola: %d",list_size(procesos_terminados));
	list_iterate(procesos_terminados, &mostrarDatosDTBlock);
	log_info(logger,"Cola procesos en ejecucion:");
	log_info(logger,"Cantidad DTBlocks en la cola: %d",list_size(procesosEnEjecucion));
	list_iterate(procesosEnEjecucion, &mostrarDatosDTBlock);
}

bool existeRecurso(void *recurso){
	Recurso *b=(Recurso*) recurso;
	if (strcmp ((*b).nombre , recursoABuscar) == 0){ //son iguales
		return true;
	}
	else
		return false;
}

bool verificarSiFinalizarElProcesoEnEjecucion(int quantum, int socketProcesoEnExec)
{
	if(bloquearDT == 1){
		   pthread_mutex_lock(&mutex_socketProcEjecucionABuscar);
		   socketProcEjecucionABuscar = socketProcesoEnExec;
		   Proceso *procesoEnExec = (Proceso*) list_find(procesosEnEjecucion,&procesoEnEjecucionEsIdABuscar);
		   list_remove_by_condition(procesosEnEjecucion,&procesoEnEjecucionEsIdABuscar);
		   pthread_mutex_unlock(&mutex_socketProcEjecucionABuscar);
		   list_add(procesos_terminados,procesoEnExec);
		   pthread_mutex_lock(&mutex_proc_ejecucion);
		   liberarRecursos(procesoEnExec);
		   pthread_mutex_unlock(&mutex_proc_ejecucion);
		   pthread_mutex_lock(&mutex_cpuBuscar);
		   idCPUBuscar = (*procesoEnExec).socketProceso;
		   ConexionCPU *cpuUtilizada = (ConexionCPU*) list_find(cpu_conectadas,&cpuUsadaEsIdABuscar);
		   (*cpuUtilizada).tieneDTBasociado = 0;
		   pthread_mutex_unlock(&mutex_cpuBuscar);
		   log_info(logger,"Desligando CPU..");
		   send((*procesoEnExec).socketProceso,"A",2,0);
		   int instructionPointer;
		   recv((*procesoEnExec).socketProceso,&instructionPointer,sizeof(int),0);
		   (*procesoEnExec).seEstaCargando = instructionPointer;  //no me importa en este caso porqe lo finalizo
//		   procesoEnEjecucion=NULL;
		   bloquearDT = 0;
//		   quantumRestante = quantum;
		   log_info(logger,"El IDBlock en ejecucion fue finalizado por orden de la consola del SAFA");
		   pthread_mutex_lock(&mutex_pcp);
		   if(flag_replanificar < (list_size(cpu_conectadas) - list_size(procesosEnEjecucion))) //nadie mando a replanificar antes
		   {
				sem_post(&sem_replanificar);
				flag_replanificar++; //se esta replanificando y ocupando otra cpu
		   }
		   pthread_mutex_unlock(&mutex_pcp);
//		   sem_post(&sem_replanificar);  //S-AFA debe replanificar
		   return true;
	}
	else
	{
		return false;
	}
}
void analizarFinEjecucionLineaProceso(int quantum,int socketProcesoEnExec)
{
	flag_quierenDesalojar=0;
    pthread_mutex_lock(&mutex_socketProcEjecucionABuscar);
	socketProcEjecucionABuscar = socketProcesoEnExec;
	Proceso *procesoEnExec = (Proceso*) list_find(procesosEnEjecucion,&procesoEnEjecucionEsIdABuscar);
	pthread_mutex_unlock(&mutex_socketProcEjecucionABuscar);
	(*procesoEnExec).quantumRestante--;
    if(!verificarSiFinalizarElProcesoEnEjecucion(quantum,socketProcesoEnExec))
    {
		if((*procesoEnExec).quantumRestante == 0){ //desaloja por fin de quantum
		   pthread_mutex_lock(&mutex_proc_ejecucion);
		   procesoEnExec->estado = listo;
		   list_remove_by_condition(procesosEnEjecucion,&procesoEnEjecucionEsIdABuscar);
		   list_add(procesos_listos,procesoEnExec); //pasa al final de la cola de listos
		   pthread_mutex_lock(&mutex_cpuBuscar);
		   idCPUBuscar = (*procesoEnExec).socketProceso;
		   ConexionCPU *cpuUtilizada = (ConexionCPU*) list_find(cpu_conectadas,&cpuUsadaEsIdABuscar);
		   (*cpuUtilizada).tieneDTBasociado = 0;
		   pthread_mutex_unlock(&mutex_cpuBuscar);
		   log_info(logger,"Desligando CPU..");
		   send((*procesoEnExec).socketProceso,"A",2,0);
		   int instructionPointer;
		   recv((*procesoEnExec).socketProceso,&instructionPointer,sizeof(int),0);
		   (*procesoEnExec).seEstaCargando = instructionPointer;  //la linea de por donde va
//		   procesoEnEjecucion=NULL;
		   pthread_mutex_unlock(&mutex_proc_ejecucion);
//		   quantumRestante = quantum;
		   log_info(logger,"Desalojo por fin de quantum");
	//	   if(list_size(procesos_listos) + list_size(procesos_bloqueados) < config_get_int_value(config, "Grado de multiprogramacion")) { //chequeo si puedo poner otro proceso en ready
	//			sem_post(&sem_cargarProceso);    //desbloquea funcion que crea el dtbDummy
	//	   }
	//	   sem_post(&sem_CPU_ejecutoUnaSentencia);  //ANALIZAR DETENIDAMENTE SI VA O NO
	//	   sem_post(&sem_replanificar);  //S-AFA debe replanificar
		   pthread_mutex_lock(&mutex_pcp);
		   if(flag_replanificar < (list_size(cpu_conectadas) - list_size(procesosEnEjecucion))) //nadie mando a replanificar antes
		   {
				sem_post(&sem_replanificar);
				flag_replanificar++; //se esta replanificando y ocupando otra cpu
		   }
		   pthread_mutex_unlock(&mutex_pcp);
		}
		else
		{
			send((*procesoEnExec).socketProceso,"1",2,0);
		}
    }
}

void liberarRecursos(Proceso *proceso)
{
	 log_info(logger,"ekjeljae");
	 if(!list_is_empty(proceso->recursosRetenidos))
	 {
		 log_info(logger,"size ee");
		 list_iterate(proceso->recursosRetenidos, &liberarRecurso);
	 }
	 log_info(logger,"Se liberaron todos los recursos del proceso");
}

void liberarRecurso(Recurso *recursoAliberar)
{
	 pthread_mutex_lock(&mutex_recursoABuscar);
//	 strcpy(recursoABuscar,(*recursoAliberar).nombre);
	 recursoABuscar = string_new();
	 string_append(&recursoABuscar,(*recursoAliberar).nombre);
	 Recurso *recurso = (Recurso*) list_find(recursos,&existeRecurso);
	 pthread_mutex_unlock(&mutex_recursoABuscar);
	 (*recurso).instancias = (*recurso).instancias + (*recursoAliberar).instancias;
	 if(list_size((*recurso).procesosEnEspera) > 0)
	 {
		 for(int i = 0; i < (*recursoAliberar).instancias; i++){
			 Proceso *procesoADesbloquear = (Proceso*) list_remove((*recurso).procesosEnEspera,i);
			 pthread_mutex_lock(&mutex_idBuscar);
			 idBuscar = (*procesoADesbloquear).idProceso;
			 procesoADesbloquear = (Proceso*) list_find(procesos_bloqueados,&procesoEsIdABuscar);
			 list_remove_by_condition(procesos_bloqueados,&procesoEsIdABuscar);
			 pthread_mutex_unlock(&mutex_idBuscar);
			 list_add(procesos_listos,procesoADesbloquear);
		 }
	 }
}

bool esArchivoBuscado(void *archivo){
	DatosArchivo *b=(DatosArchivo*) archivo;
	if (strcmp ((*b).nombre , archivoAbuscar) == 0){ //son iguales
		return true;
	}
	else
		return false;
}


bool noTieneDTBasociado(void *cpuConectada){
	ConexionCPU *b=(ConexionCPU*) cpuConectada;
	if((*b).tieneDTBasociado == 0){
		return true;
	}
	else
		return false;
}

bool procesoEnEjecucionEsIdABuscar(void * proceso){
	Proceso *proc=(Proceso*) proceso;
	if((*proc).socketProceso==socketProcEjecucionABuscar)
		return true;
	else
		return false;
}

