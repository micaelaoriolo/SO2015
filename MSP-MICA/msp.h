/*
 * msp.h
 *
 *  Created on: 26/09/2014
 *      Author: utnso
 */

#ifndef MSP_H_
#define MSP_H_

#include <stdbool.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <commons/sockets.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <pthread.h>
#include <commons/temporal.h>
#include <unistd.h>
#include <math.h>

#define CANTIDAD_MAX_SEGMENTOS_POR_PID 4096
#define CANTIDAD_MAX_PAGINAS_POR_SEGMENTO 4096
#define TAMANIO_PAGINA 256



//ESTRUCTURAS

//Esta estructura me sirve para guardar todos los parametros de configuracion
typedef struct
{
	int puerto;
	int cantidad_memoria;
	int cantidad_swap;
	char* sust_pags;
} t_configuracion;


typedef struct
{
	int tamanio;
	int pid;
	int numeroSegmento;
	t_list* listaPaginas;
} nodo_segmento;

typedef struct
{
	int nro_marco;
	int pid;
	int nro_segmento;
	int nro_pagina;
	void* dirFisica;
	int libre;	//si vale 1, el marco está ocupado, si vale 0 está libre
	int orden;
	int referencia; //si vale 1 se ha usado recientemente
	int modificacion; //si vale 1 se ha modificado recientemente
	pthread_rwlock_t rwMarco;
} t_marco;

typedef struct
{
	int nro_pagina;
	int presencia;	//si vale -1, no se le asignó ningun marco, si vale -2 está en swap, si no indica numero de marco
	pthread_rwlock_t rwPagina;
} nodo_paginas;

//VARIABLES globales

int ordenMarco;

t_list* listaSegmentos;

t_configuracion configuracion;

t_log *logs;

void* memoriaPrincipal;

t_marco *tablaMarcos;

int memoriaRestante;

int swapRestante;

int cantidadMarcos;

int consola;

int tamanioRestanteTotal;

int puntero;

pthread_mutex_t mutexMemoriaTotalRestante;
pthread_mutex_t mutexSwapRestante;
pthread_mutex_t mutexMPRestante;
pthread_mutex_t mutexPuntero;
pthread_mutex_t mutexOrdenMarco;

pthread_rwlock_t rwListaSegmentos;


pthread_t hilo_consola_1;

/*
typedef enum
{
	ENUM_FANTASMA,
	VIOLACION_DE_SEGMENTO,
	DIRECCION_INVALIDA,
	PID_INEXISTENTE,
	MEMORIA_INSUFICIENTE,
	DIRECCION_VALIDA,
	PID_EXCEDE_CANT_MAX_SEGMENTO,
	TAMANIO_NEGATIVO,
	SEGMENTO_EXCEDE_TAM_MAX
} t_error;
*/


//FUNCIONES

void tablaPaginas(int pid); //revisada

t_list* filtrarListaSegmentosPorPid(int pid); //revisada

t_list* crearListaPaginas(int cantidadDePaginas); //revisada

uint32_t crearSegmento(int pid, int tamanio); //revisada

uint32_t agregarSegmentoALista(int cantidadDePaginas, int pid, int cantidadDeSegmentosDeEstePid, int tamanio); //revisada

void tablaSegmentos(); //revisada

void levantarArchivoDeConfiguracion(); //revisada

void inicializarMSP(); //revisada

int consola_msp();

int buscarComando(char* buffer);

void liberarSubstrings(char** sub);

void listarMarcos(); //revisada

void crearTablaDeMarcos(); //revisada

uint32_t generarDireccionLogica(int numeroSegmento, int numeroPagina, int offset); //revisada

void obtenerUbicacionLogica(uint32_t direccion, int *numeroSegmento, int *numeroPagina, int *offset); //revisada

int destruirSegmento(int pid, uint32_t base); //revisada

nodo_segmento* buscarNumeroSegmento(t_list* listaSegmentosFiltradosPorPID, int numeroSegmento); //revisada

void* buscarYAsignarMarcoLibre(int pid, int numeroSegmento, nodo_paginas *nodoPagina);

t_list* validarEscrituraOLectura(int pid, uint32_t direccionLogica, int tamanio);

t_list* paginasQueVoyAUsar(nodo_segmento *nodoSegmento, int numeroPagina, int cantidadPaginas); //revisada

int escribirMemoria(int pid, uint32_t direccionLogica, void* bytesAEscribir, int tamanio); //revisada

void escribirEnMarco(int numeroMarco, int tamanio, void* bytesAEscribir, int offset, int yaEscribi); //revisada

void *solicitarMemoria(int pid, uint32_t direccionLogica, int tamanio);

void conexionConKernelYCPU();

void* atenderACPU(void *socket_cpu);

void* atenderAKernel(void* socket_msp);

int crearArchivoDePaginacion(int pid, int numeroSegmento, nodo_paginas *nodoPagina); //revisada

char* generarNombreArchivo(int pid, int numeroSegmento, int numeroPagina); //revisada

void elegirVictimaSegunFIFO(); //revisada

void elegirVictimaSegunClockM();

void liberarMarco(int numeroMarco, nodo_paginas *nodoPagina); //revisada

uint32_t aumentarProgramCounter(uint32_t programCounterAnterior, int bytesASumar);

void moverPaginaDeSwapAMemoria(int pid, int segmento, nodo_paginas *nodoPagina);

void swappearDeMemoriaADisco(t_marco nodoMarco);

int primeraVueltaClock(int puntero);

int segundaVueltaClock(int puntero);

void terminarMSP();

int direccionInValida(uint32_t direccionLogica, int pid, int tamanio);

int mandarErrorkernel (int socket_kernel, int dirLogica, int pid, int tamanio);

int mandarErrorCPU (int socket_cpu, int dirLogica, int pid, int tamanio);





#endif /* MSP_H_ */
