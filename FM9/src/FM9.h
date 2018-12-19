/*
 * FM9.h
 *
 *  Created on: 24/10/2018
 *      Author: utnso
 */

#ifndef FM9_H_
#define FM9_H_
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <FuncionesConexiones.h>
#include <commons/log.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <pthread.h>
#include <semaphore.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <commons/string.h>
#include <commons/config.h>
//#include <commons/memory.h>

/* amount of hex columns of dump */
  #define HEXDUMP_COLS 16
  /* amount of hex columns allow without a separator */
  #define HEXDUMP_COLS_SEP 8

typedef struct {
	int32_t tamanio;
	int32_t dirReal;
	int32_t inicioEnTabla;
	int32_t pID;
	int32_t idSg;
//	t_puntero dirV;
} tNodoBloques;

typedef struct {
	int idProceso;
	int numeroSegmento;
	int base;
	int limite;
} SegmentoDelProceso;

typedef struct {
	int idProceso;
	int numeroSegmento;
	t_list *paginas;
} SegmentoPaginadoDelProceso;

typedef struct {
	int numeroPagina;
	int numeroMarco;
} Pagina ;

typedef struct {
	int numeroMarco;
	int direccionBase;
	bool estaLibre;
} Marco ;

typedef struct {
	int numeroSegmento; //direccion logica base de la posicion dentro de la memoria
	int base; //direccion real de la posicion dentro de la memoria e incluye
	int limite; //el limite es la cantidad maximas de lineas que tiene es excluyente el valor
	int CantBytes; // tamaño del archivo en Bytes
	bool estaLibre;
} SegmentoDeTabla;

typedef struct {
	int idProceso;
	t_list *segmentosDelProceso;
} TablaSegmentosDeProceso;

t_list *lista_segmentos;
t_list *lista_procesos;
t_list *lista_marcos;
pthread_t* hilosPedidosDAM;
pthread_t hiloDAM;
sem_t sem_esperaInicializacion;
pthread_mutex_t mutex_segmentoABuscar;
pthread_mutex_t mutex_segmentoPaginadoABuscar;
pthread_mutex_t mutex_idTablaABuscar;
pthread_mutex_t mutex_ListaMarcos;
pthread_mutex_t mutex_direccionBaseAusar;
pthread_mutex_t mutex_infoArchivoFlush;
int tamanioMemoria;
int tamanioMaxLinea;
char* memoriaReal;
int direccionBaseProxima ;
int direccionBaseAusar ; //direccion real de la posicion dentro de la memoria
int tamanioArchivoAMeter;
int numSegmentoAbuscar;
int idGlobal;
int idTablaBuscar;
int idProcesoAbuscar;
int idMarcoGlobal;
int cantidadMaximaMarcos;
int numSegmentoPaginadoAbuscar;
char* infoArchivoFlush;
int numArchivo;

void* conexionCPU(void* nuevoCliente);
void* conexionDAM(void* nuevoCliente);
int agregarSegmento(char *nuevoEscriptorio,Paquete_DAM* paqueteDAM);
char* obtenerInfoSegmento(int numeroSegmento,int idProcesoSolicitante);
char* obtenerInfoSegmentoPaginado(int numeroSegmento,int idProcesoSolicitante);
int asignarModificarArchivoEnMemoria(Paquete_DAM* paqueteDAM);
int liberarMemoria(Paquete_DAM* paqueteDAM);
bool esProcesoAbuscar(void * proceso);
bool segmentoEsIdABuscar(void * segmento);
bool entraArchivoEnSegmento(void *segmentoDeTabla);
int dameProximaDireccionBaseDisponible(int tamanio, int *numSegmentoNuevo,int i);
int agregarSegmentoPaginado(char *nuevoEscriptorio,Paquete_DAM * paquete);
int agregarMarcoAtablaGeneral (Pagina* nuevaPagina,char *nuevoEscriptorio,int posicionEnArchivo);
bool segmentoPaginadoEsIdABuscar(void * segmento);
void liberarMarcoAsociado(void * pagina);
void extraerInfoMarcoAsociado(void * pagina);
bool marcoEstaLibre(void *marcoDeTabla);
void cambiarNumeroSegmento(Paquete_DAM* paqueteDAM,int numeroSegmentoOriginal);
void liberarSegmento(SegmentoDelProceso *segmentoALiberar);
void liberarSegmentoPaginado(SegmentoPaginadoDelProceso *segmentoPaginadoALiberar);
int paginarSegmento(SegmentoPaginadoDelProceso *nuevoSegmentoPaginadoDelProceso,char *nuevoEscriptorio);
void guardarInfoEnMemoria(int direccionBaseMarco,char *nuevoEscriptorio, int posicionEnArchivo);
char *mem_hexstring(void *source, size_t length);
void* consola();
int cantidadDeCentinelas(char** centinelas);
void dumpSegmentacionPura(int IDDTB);
void dumpSegmentacionPaginada(int IDDTB);
void mostrarInfoSegmento(SegmentoDelProceso *segmento);
void mostrarInfoSegmentoPaginado(SegmentoPaginadoDelProceso *segmentoPaginado);
#endif /* FM9_H_ */

