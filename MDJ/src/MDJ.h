/*
 * MDJ.h
 *
 *  Created on: 1 nov. 2018
 *      Author: utnso
 */

#ifndef MDJ_H_
#define MDJ_H_

#include <pthread.h>
#include <semaphore.h>
#include <commons/bitarray.h>
#include <commons/collections/list.h>
#include <commons/config.h>

//int socketDAM;
t_config *configMDJ;
int retardo;
char* puntoMontaje;
int puerto;
char* rutaBloque;
char* rutaBitMap;
char* rutaMetadata;
//char* rutaArchivoActual;
int tamanioBloque;
int cantidadBloques;
char* map;
int sizeBitmap;
sem_t sem_esperaInicializacion;
t_list *pedidosDAM ;
t_bitarray *bitmap;
pthread_t hiloAtencionDAM;
pthread_t hiloRecepcionPedidosDAM;
pthread_t* hilosPedidosDAM;
pthread_mutex_t mutex_bitmap;

void* atenderDAM(void* nuevoCliente);
void* recepcionPedidosDAM(void* nuevoCliente);
int validarArchivo(char* nombreArchivo, int* size);
char* obtenerDatosArchivo(char* nombreArchivo ,int offset, int size);
int borrarArchivo(char* nombreArchivo);
int crearArchivo(char* nombreArchivo,int cantBytes);
int guardarDatosArchivo(char* nombreArchivo, int offset, int size, char* codigoArchivo);
void testeoMDJ();
void inicializarVariables();
int asignarNuevosBloques(char *codigoArchivo,int posicion,int cantBloquesFaltantes, int cantidadBloquesTotales, char** bloques, int nuevoTamanioArchivo,char* rutaArchivoActual);
void actualizarScriptArchivo(char** bloques,t_list* bloqueNuevo,int nuevoTamanioArchivo,char* rutaArchivoActual);
void test();
void crearDirectorio(char* path);
#endif /* MDJ_H_ */
