/*
 * Consola_MDJ.h
 *
 *  Created on: 11 dic. 2018
 *      Author: utnso
 */

#ifndef CONSOLA_MDJ_H_
#define CONSOLA_MDJ_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <commons/string.h>
#include <commons/log.h>
#include <dirent.h>
#include <openssl/md5.h>

char *puntoInicialConsola;
char *pathActual;
void* consola();
int recorrerCentinela(char** centinelas,int liberar);
int cantidadDeCentinelas(char** centinelas);
int validarArchivo2(char* nombreArchivo, int* size);
char* obtenerDatosArchivo2(char* nombreArchivo ,int offset, int size);
void ls_sinParametro();
void ls(char* pathDirectorio);
void cd(char* pathDirectorio);
void md5(char* pathArchivo);

#endif /* CONSOLA_MDJ_H_ */
