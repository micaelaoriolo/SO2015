
#ifndef SOCKETS_H
#define SOCKETS_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <unistd.h>

/*Commons includes*/
#include <string.h>
#include "log.h"
#include "string.h"

#define MSG_SIZE 256
#define STRUCT_SIZE (MSG_SIZE + sizeof(t_header))
#define BACKLOG 10 // cantidad máxima de sockets a mantener en pool de espera

typedef struct sockaddr_in tSocketInfo;
typedef struct sockaddr_in t_socket_info;


// Ejemplos:
// printf("%sHello, %sworld!%s\n", red, blue, none);
// printf("%sHello%s, %sworld!\n", green, none, cyan);
// printf("%s", none);


char* getDescription(int item);

/**
 * Nomenclatura de los mensajes:
 * 'PROC1_TO_PROC2_TIPO_DE_MENSAJE' // Params: 'A', 'B', 'C'
 *  y el proceso que lo reciba deberá esperar los parámetros 'A', 'B', 'C'
 */
typedef enum {
	ERR_CONEXION_CERRADA,       // evaluar solo este mensaje por si se cierra una conexion
	ERR_ERROR_AL_RECIBIR_MSG,
	ERR_ERROR_AL_ENVIAR_MSG,
	CODE_HAY_MAS_LINEAS,
	CODE_HAY_MAS_LINEAS_OK,
	CODE_FIN_LINEAS,
	PRG_TO_KRN_HANDSHAKE,
	PRG_TO_KRN_CODE,
	KERNEL_TO_CPU_HANDSHAKE,
	KERNEL_TO_CPU_TCB,
	KERNEL_TO_CPU_TCB_MODO_KERNEL,
	KERNEL_TO_CPU_VAR_COMPARTIDA_OK,
	KERNEL_TO_CPU_VAR_COMPARTIDA_ERROR,
	KERNEL_TO_CPU_ASIGNAR_OK,
	KERNEL_TO_CPU_BLOCKED,
	KERNEL_TO_CPU_OK,
	KERNEL_TO_CPU_TCB_QUANTUM,
	KERNEL_TO_PRG_IMPR_PANTALLA,  // imprimir en pantalla
	KERNEL_TO_PRG_IMPR_VARIABLES, // imprimir valores finales de variables
	KERNEL_TO_PRG_END_PRG,
	KERNEL_TO_PRG_NO_MEMORY,
	KERNEL_TO_MSP_MEM_REQ,
	KERNEL_TO_MSP_ELIMINAR_SEGMENTOS,
	KERNEL_TO_MSP_HANDSHAKE,
	KERNEL_TO_MSP_ENVIAR_BYTES,
	KERNEL_TO_MSP_ENVIAR_ETIQUETAS,
	KERNEL_TO_MSP_SOLICITAR_BYTES,
	CPU_TO_MSP_INDICEYLINEA,
	CPU_TO_MSP_HANDSHAKE,
	CPU_TO_MSP_CAMBIO_PROCESO,
	CPU_TO_MSP_SOLICITAR_BYTES,
	CPU_TO_MSP_ENVIAR_BYTES,
	CPU_TO_MSP_CREAR_SEGMENTO,
	CPU_TO_MSP_DESTRUIR_SEGMENTO,
	CPU_TO_KERNEL_HANDSHAKE,
	CPU_TO_MSP_GUARDA_CADENA_EN_MEMORIA,
	CPU_TO_KERNEL_NEW_CPU_CONNECTED,
	CPU_TO_KERNEL_END_PROC,
	CPU_TO_KERNEL_END_PROC_ERROR,
	CPU_TO_KERNEL_FINALIZO_QUANTUM_NORMALMENTE,
	CPU_TO_KERNEL_END_PROC_QUANTUM_SIGNAL,
	CPU_TO_KERNEL_IMPRIMIR,
	CPU_TO_KERNEL_IMPRIMIR_TEXTO,
	CPU_TO_KERNEL_OK,
	CPU_TO_KERNEL_INTERRUPCION_BLOQUEAR_PROCESO,
	MSP_TO_KERNEL_MEMORY_OVERLOAD, // No existe espacio suficiente para crear un segmento
	MSP_TO_KERNEL_SEGMENTO_CREADO,
	MSP_TO_KERNEL_ENVIO_BYTES,
	MSP_TO_KERNEL_HANDSHAKE_OK,
	MSP_TO_CPU_BYTES_ENVIADOS,
	MSP_TO_CPU_BYTES_RECIBIDOS,
	MSP_TO_CPU_SENTENCE,
	MSP_TO_CPU_SEGM_FAULT,
	SYSCALL_SIGNAL_REQUEST,
	SYSCALL_WAIT_REQUEST,// El acceso a memoria esta por fuera de los rangos permitidos.
	CPU_TO_MSP_PARAMETROS,
	CPU_TO_KERNEL_INTERRUPCION,
	CPU_TO_KERNEL_CREAR_HILO,
	CPU_TO_KERNEL_ENTRADA_ESTANDAR,
	KERNEL_TO_CONSOLA_ENTRADA_ESTANDAR,
	CONSOLA_ENVIAR_MENSAJE_KERNEL,
	CPU_TO_KERNEL_SALIDA_ESTANDAR,
	KERNEL_TO_CONSOLA_SALIDA_ESTANDAR,
	CPU_TO_KERNEL_JOIN,
	CPU_TO_KERNEL_BLOCK,
	CPU_TO_KERNEL_WAKE,
	CPU_TO_MSP_ESCRIBIR_MEMORIA,
	MSP_TO_CPU_PID_INVALIDO,
	MSP_TO_CPU_VIOLACION_DE_SEGMENTO,
	MSP_TO_CPU_DIRECCION_INVALIDA,
	MSP_TO_CPU_MEMORIA_INSUFICIENTE,
	MSP_TO_CPU_TAMANIO_NEGATIVO,
	MSP_TO_CPU_SEGMENTO_EXCEDE_TAMANIO_MAXIMO,
	MSP_TO_CPU_PID_EXCEDE_CANT_MAXIMA_DE_SEGMENTOS,
	MSP_TO_CPU_SEGMENTO_CREADO,
	MSP_TO_CPU_SEGMENTO_DESTRUIDO,
	MSP_TO_KERNEL_DIRECCION_INVALIDA,
	MSP_TO_KERNEL_MEMORIA_INSUFICIENTE,
	MSP_TO_KERNEL_PID_INVALIDO,
	MSP_TO_KERNEL_SEGMENTO_DESTRUIDO,
	MSP_TO_KERNEL_SEGMENTO_EXCEDE_TAMANIO_MAXIMO,
	MSP_TO_KERNEL_PID_EXCEDE_CANT_MAXIMA_DE_SEGMENTOS,
	MSP_TO_KERNEL_TAMANIO_NEGATIVO,
	MSP_TO_KERNEL_VIOLACION_DE_SEGMENTO,
	MSP_TO_KERNEL_MEMORIA_ESCRITA,
	MSP_TO_KERNEL_ESPERO_BYTES,
	MSP_TO_CPU_ENVIO_BYTES,
	VIOLACION_DE_SEGMENTO,
	DIRECCION_INVALIDA,
	PID_INEXISTENTE,
	MEMORIA_INSUFICIENTE,
	DIRECCION_VALIDA,
	PID_EXCEDE_CANT_MAX_SEGMENTO,
	TAMANIO_NEGATIVO,
	SEGMENTO_EXCEDE_TAM_MAX,
	CPU_TO_MSP_ESCRIBIR_MEMORIA_NUMERO,
	CPU_TO_MSP_SOLICITAR_NUMERO,



} t_header;



/* Defino los tipos de señales que se pueden mandar
 *
 * Sintaxis correcta para escribir una nueva señal:
 *
 * 	#define [Origen]_TO_[Destino]_[Mensaje] numeroIncremental
 *
 * 	Donde:
 * 		[Origen] es quien manda el mensaje
 * 		[Destino] es quien recive el mensaje
 * 		[Mensaje] lo que se le quiere mandar
 * 		numeroIncrementar un numero mas que la señal anterior
 */

typedef char t_contenido[MSG_SIZE];

typedef struct ts_mensajes {
	t_header id;
	t_contenido contenido;
}t_mensajes;


/*
 * crearSocket: Crea un nuevo socket.
 * @return: El socket creado.
 * 			EXIT_FAILURE si este no pudo ser creado.
 */
int crearSocket();


/*
 * bindearSocket: Vincula un socket con una direccion de red.
 *
 * @arg socket:		Socket a ser vinculado.
 * @arg socketInfo: Protocolo, direccion y puerto de escucha del socket.
 * @return:			0 si el socket ha sido vinculado correctamente.
 * 					EXIT_FAILURE si se genera error.
 */
int bindearSocket(int unSocket, tSocketInfo socketInfo);


/*
 * escucharEn: Define el socket de escucha.
 *
 * @arg socket:		Socket de escucha.
 * @return:			EXIT_SUCCESS si la conexion es exitosa.
 * 					EXIT_FAILURE si se genera error.
 */
int escucharEn(int unSocket);


/*
 * conectarServidor: Crea una conexion a un servidor como cliente.
 *
 * @arg puertoDestino: Puerto de escucha del servidor.
 * @arg ipDestino: 	   IP del servidor.
 * @return: 		   int: Socket de conexión con el server.
 *					   EXIT_FAILURE: No se pudo conectar.
 */
int conectarAServidor(char *ipDestino, unsigned short puertoDestino);


/*
 * desconectarseDe: Se desconecta el socket especificado.
 *
 * @arg socket: Socket al que estamos conectados.
 * @return:		EXIT_SUCCESS: La desconexion fue exitosa.
 * 				EXIT_FAILURE: Se genero error al desconectarse.
 */
int desconectarseDe(int socket);

/*
 * Funcion enviar mensaje, recibe como parámetros:
 * socket, el header y el mensaje propiamente dicho
 * devuelve el numero de bytes enviados
 */
int enviarMensaje(int numSocket, t_header header, t_contenido mensaje, t_log *logger);

/*lo mismo pero para codigo*/
int32_t enviarCodigo(int32_t numSocket, t_header header, t_contenido initialMessage, char* stringCode, t_log *logger, bool NoBrNoComment);

/*
 * Funcion recibir mensaje, recive como parámetros:
 * socket y mensaje donde se pondŕa en mensaje,
 * y devuelve el header, para evaluar la accion a tomar.
 */
t_header recibirMensaje(int numSocket, t_contenido mensaje, t_log *logger);

/*lo mismo pero para codigo*/
char* recibirCodigo(int numSocket, t_header authorizedHeader, t_log *logger);

//Funciones auxiliares
char* separarLineas(char* codigo);

/*
 * Funcion cerrar un socket, y eliminarlo de un fd_set[opcional]:
 * retorna EXIT_SUCCESS si se realizó bien, EXIT_FAILURE sino.t_header recibirCodigo(int numSocket, t_header authorizedHeader, char** stringCode, t_log *logger)
 * ej: cerrarSocket(mySocket, &read_fds);t_header recibirCodigo(int numSocket, t_header authorizedHeader, char**t_header recibirCodigo(int numSocket, t_header authorizedHeader, char** stringCode, t_log *logger) stringCode, t_log *logger)
 */
int cerrarSocket(int numSocket, fd_set* fd);


//UTILIDADES QUE USA MARIANO

#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>

#define ERROR		    0
#define EXITO		    1
#define WARNING         2

#define OTRO			9

#pragma pack(1)
typedef struct header_s
{
  char id[16];
  char tipo;
  int  largo_mensaje;

}header_t;
#pragma pack(0)

header_t* crearHeader();

int enviar(int sock, char *buffer, int tamano);
int recibir(int sock, char *buffer, int tamano);

#define ERROR_SOCK		    0
#define EXITO_SOCK		    1
#define WARNING_SOCK         2

#endif
