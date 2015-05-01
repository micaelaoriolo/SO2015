
#include "sockets.h"

int crearSocket() {

	int newSocket;
	int si = 1;
	if ((newSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return EXIT_FAILURE;
	} else {
		if (setsockopt(newSocket, SOL_SOCKET, SO_REUSEADDR, &si, sizeof(int)) == -1) {
			return EXIT_FAILURE;
		}
		return newSocket;
	}
}


int bindearSocket(int newSocket, tSocketInfo socketInfo) {
	if (bind(newSocket, (struct sockaddr*)&socketInfo, sizeof(socketInfo)) == -1) {
		perror("Error al bindear socket escucha");
		return EXIT_FAILURE;
	} else {
		return EXIT_SUCCESS;
	}
}


int escucharEn(int newSocket) {
	if (listen(newSocket, BACKLOG) == -1) {
		perror("Error al poner a escuchar socket");
		return EXIT_FAILURE;
	} else {
		return EXIT_SUCCESS;
	}
}


int conectarAServidor(char *ipDestino, unsigned short puertoDestino) {
	int socketDestino;
	tSocketInfo infoSocketDestino;
	infoSocketDestino.sin_family = AF_INET;
	infoSocketDestino.sin_port = htons(puertoDestino);
	inet_aton(ipDestino, &infoSocketDestino.sin_addr);
	memset(&(infoSocketDestino.sin_zero), '\0', 8);

	if ((socketDestino = crearSocket()) == EXIT_FAILURE) {
		perror("Error al crear socket");
		return EXIT_FAILURE;
	}

	if (connect(socketDestino, (struct sockaddr*) &infoSocketDestino, sizeof(infoSocketDestino)) == -1) {
		perror("Error al conectar socket");
		close(socketDestino);
		return EXIT_FAILURE;
	}

	return socketDestino;
}

int desconectarseDe(int socket) {
	if (close(socket)) {
		return EXIT_SUCCESS;
	} else {
		return EXIT_FAILURE;
	}
}

int32_t enviarMensaje(int32_t numSocket, t_header header, t_contenido mensaje, t_log *logger) {

	if(strlen(mensaje)>sizeof(t_contenido)){
		log_error(logger, "Error en el largo del mensaje, tiene que ser menor o igual al máximo: %s",MSG_SIZE);
		return EXIT_FAILURE;
	}

	t_mensajes *s = malloc(sizeof(t_mensajes));
	memset(s, 0, sizeof(t_mensajes));

	s->id = header;
	strcpy(s->contenido, mensaje);
	log_info(logger, "Se ENVIA por SOCKET:%d - HEADER:%s MENSAJE:\"%s\" ",
			numSocket, getDescription(s->id), s->contenido);
	int n = send(numSocket, s, sizeof(*s), 0);
	if(n != STRUCT_SIZE){
		log_error(logger, "#######################################################################");
		log_error(logger, "##    ERROR en el envío de mensajes: no se envió todo. socket: %d    ##", numSocket);
		log_error(logger, "#######################################################################");
	}
	free(s);
	return n;
}

t_header recibirMensaje(int numSocket, t_contenido mensaje, t_log *logger) {

	char buffer[STRUCT_SIZE];

	log_debug(logger, "Estoy en espera");
	int n = recv(numSocket, buffer, STRUCT_SIZE, 0);

	if (n == STRUCT_SIZE) {
		t_mensajes* s = (t_mensajes*)buffer;
		strcpy(mensaje, s->contenido);
		log_debug(logger, "Se RECIBE por SOCKET:%d - HEADER:%s MENSAJE:\"%s\" ",
				numSocket, getDescription(s->id),mensaje);
		return s->id;
	} else {
		if (n == 0) { // Conexión remota cerrada
			log_info(logger, "El socket %d cerró la conexion.", numSocket);
			strcpy(mensaje, "");
			return ERR_CONEXION_CERRADA;
		} else { // El mensaje tiene un tamaño distinto al esperado
			log_error(logger, "#######################################################");
			log_error(logger, "##    ERROR en el recibo de mensaje del socket %d    ##", numSocket);
			log_error(logger, "#######################################################");
			strcpy(mensaje, "");
			//usleep(500000);
			return ERR_ERROR_AL_RECIBIR_MSG;
		}
	}
}

int cerrarSocket(int numSocket, fd_set *fd){

	close(numSocket);

	if (fd!=NULL) {
		FD_CLR(numSocket,fd);
	}
	return EXIT_SUCCESS;
}

char* getDescription(int item){

	switch(item){
	case 	ERR_CONEXION_CERRADA: 	return 	"ERR_CONEXION_CERRADA  "	;
	case	ERR_ERROR_AL_RECIBIR_MSG:	return 	"ERR_ERROR_AL_RECIBIR_MSG "	;
	case	ERR_ERROR_AL_ENVIAR_MSG:	return 	"ERR_ERROR_AL_ENVIAR_MSG "	;
	case	CODE_HAY_MAS_LINEAS:	return 	"CODE_HAY_MAS_LINEAS "	;
	case 	CODE_HAY_MAS_LINEAS_OK: return "CODE_HAY_MAS_LINEAS_OK";
	case	CODE_FIN_LINEAS:	return 	"CODE_FIN_LINEAS "	;
	case	PRG_TO_KRN_CODE:	return 	"PRG_TO_KRN_CODE "	;
	case	PRG_TO_KRN_HANDSHAKE:	return 	"PRG_TO_KRN_HANDSHAKE"	;
	case	KERNEL_TO_CPU_HANDSHAKE:	return 	"KERNEL_TO_CPU_HANDSHAKE "	;
	case	KERNEL_TO_CPU_TCB:	return 	"KERNEL_TO_CPU_TCB"	;
	case	KERNEL_TO_CPU_VAR_COMPARTIDA_OK:	return 	"KERNEL_TO_CPU_VAR_COMPARTIDA_OK "	;
	case	KERNEL_TO_CPU_VAR_COMPARTIDA_ERROR:	return 	"KERNEL_TO_CPU_VAR_COMPARTIDA_ERROR "	;
	case	KERNEL_TO_CPU_ASIGNAR_OK:	return 	"KERNEL_TO_CPU_ASIGNAR_OK "	;
	case	KERNEL_TO_CPU_BLOCKED:	return 	"KERNEL_TO_CPU_BLOCKED "	;
	case	KERNEL_TO_CPU_OK:	return 	"KERNEL_TO_CPU_OK "	;
	case	KERNEL_TO_CPU_TCB_QUANTUM:	return 	"KERNEL_TO_CPU_TCB_QUANTUM "	;
	case	KERNEL_TO_PRG_IMPR_PANTALLA:	return 	"KERNEL_TO_PRG_IMPR_PANTALLA "	;
	case	KERNEL_TO_PRG_END_PRG:	return 	"KERNEL_TO_PRG_END_PRG "	;
	case	KERNEL_TO_PRG_NO_MEMORY:	return 	"KERNEL_TO_PRG_NO_MEMORY "	;
	case	KERNEL_TO_MSP_MEM_REQ:	return 	"KRN_TO_MSP_MEM_REQ "	;
	case	KERNEL_TO_MSP_ELIMINAR_SEGMENTOS:	return 	"KERNEL_TO_UMV_ELIMINAR_SEGMENTOS "	;
	case	KERNEL_TO_MSP_HANDSHAKE:	return 	"KERNEL_TO_MSP_HANDSHAKE "	;
	case	KERNEL_TO_MSP_ENVIAR_BYTES:	return 	"KRN_TO_MSP_ENVIAR_BYTES "	;
	case	KERNEL_TO_MSP_ENVIAR_ETIQUETAS:	return 	"KERNEL_TO_MSP_ENVIAR_ETIQUETAS "	;
	case	KERNEL_TO_MSP_SOLICITAR_BYTES:	return 	"KERNEL_TO_MSP_SOLICITAR_BYTES "	;

	case	CPU_TO_MSP_INDICEYLINEA:	return 	"CPU_TO_MSP_INDICEYLINEA "	;
	case	CPU_TO_MSP_HANDSHAKE:	return 	"CPU_TO_MSP_HANDSHAKE "	;
	case	CPU_TO_MSP_CAMBIO_PROCESO:	return 	"CPU_TO_MSP_CAMBIO_PROCESO "	;
	case	CPU_TO_MSP_SOLICITAR_BYTES:	return 	"CPU_TO_MSP_SOLICITAR_BYTES"	;

	case    CPU_TO_MSP_ENVIAR_BYTES: return "CPU_TO_MSP_ENVIAR_BYTES";
	case	CPU_TO_KERNEL_HANDSHAKE:	return 	"CPU_TO_KERNEL_HANDSHAKE "	;
	case	CPU_TO_KERNEL_NEW_CPU_CONNECTED:	return 	"CPU_TO_KERNEL_NEW_CPU_CONNECTED "	;
	case	CPU_TO_KERNEL_END_PROC:	return 	"CPU_TO_KERNEL_END_PROC "	;
	case	CPU_TO_KERNEL_FINALIZO_QUANTUM_NORMALMENTE:	return 	"CPU_TO_KERNEL_END_PROC_QUANTUM "	;
	case	CPU_TO_KERNEL_END_PROC_QUANTUM_SIGNAL: return "CPU_TO_KERNEL_END_PROC_QUANTUM_SIGNAL";
	case 	CPU_TO_KERNEL_END_PROC_ERROR:		return "CPU_TO_KERNEL_END_PROC_ERROR";
	case	CPU_TO_KERNEL_IMPRIMIR:	return 	"CPU_TO_KERNEL_IMPRIMIR "	;
	case	CPU_TO_KERNEL_IMPRIMIR_TEXTO:	return 	"CPU_TO_KERNEL_IMPRIMIR_TEXTO "	;
	case	CPU_TO_KERNEL_OK:	return 	"CPU_TO_KERNEL_OK "	;
	case	MSP_TO_KERNEL_MEMORY_OVERLOAD:	return 	"MSP_TO_KERNEL_MEMORY_OVERLOAD "	;
	case	MSP_TO_KERNEL_SEGMENTO_CREADO:	return 	"MSP_TO_KERNEL_SEGMENTO_CREADO "	;
	case	MSP_TO_KERNEL_ENVIO_BYTES:	return 	"MSP_TO_KERNEL_ENVIO_BYTES "	;
	case	MSP_TO_KERNEL_HANDSHAKE_OK:	return 	"MSP_TO_KERNEL_HANDSHAKE_OK "	;
	case	MSP_TO_CPU_BYTES_ENVIADOS:	return 	"MSP_TO_CPU_BYTES_ENVIADOS "	;
	case	MSP_TO_CPU_BYTES_RECIBIDOS:	return 	"MSP_TO_CPU_BYTES_RECIBIDOS "	;
	case	MSP_TO_CPU_SENTENCE:	return 	"MSP_TO_CPU_SENTENCE "	;
	case	MSP_TO_CPU_SEGM_FAULT: 	return 	"MSP_TO_CPU_SEGM_FAULT  "	;
	case    MSP_TO_CPU_VIOLACION_DE_SEGMENTO : return "MSP_TO_CPU_VIOLACION_DE_SEGMENTO";
	case    MSP_TO_CPU_DIRECCION_INVALIDA : return "MSP_TO_CPU_DIRECCION_INVALIDA";
	case    MSP_TO_CPU_PID_INVALIDO : return "MSP_TO_CPU_PID_INVALIDO";
	case	SYSCALL_WAIT_REQUEST: 	return 	"SYSCALL_WAIT_REQUEST  "	;
	case	SYSCALL_SIGNAL_REQUEST: 	return 	"SYSCALL_SIGNAL_REQUEST  "	;
	case    CPU_TO_MSP_DESTRUIR_SEGMENTO: return "CPU_TO_MSP_DESTRUIR_SEGMENTO";
	case    CPU_TO_MSP_PARAMETROS : return "CPU_TO_MSP_PARAMETROS";
	case    CPU_TO_KERNEL_INTERRUPCION : return "CPU_TO_KERNEL_INTERRUPCION";
	case    CPU_TO_KERNEL_CREAR_HILO : return "CPU_TO_KERNEL_CREAR_HILO";
	case    CPU_TO_KERNEL_ENTRADA_ESTANDAR : return "CPU_TO_KERNEL_ENTRADA_ESTANDAR";
	case    KERNEL_TO_CONSOLA_ENTRADA_ESTANDAR : return "KERNEL_TO_CONSOLA_ENTRADA_ESTANDAR";
	case    CONSOLA_ENVIAR_MENSAJE_KERNEL : return "CONSOLA_ENVIAR_MENSAJE_KERNEL";
	case    CPU_TO_KERNEL_SALIDA_ESTANDAR : return "CPU_TO_KERNEL_SALIDA_ESTANDAR";
	case    KERNEL_TO_CONSOLA_SALIDA_ESTANDAR : return "KERNEL_TO_CONSOLA_SALIDA_ESTANDAR";
	case    CPU_TO_MSP_ESCRIBIR_MEMORIA : return "CPU_TO_MSP_ESCRIBIR_MEMORIA";
	case    KERNEL_TO_PRG_IMPR_VARIABLES : return "KERNEL_TO_PRG_IMPR_VARIABLES";


	case MSP_TO_CPU_MEMORIA_INSUFICIENTE : return "MSP_TO_CPU_MEMORIA_INSUFICIENTE";
	case MSP_TO_CPU_TAMANIO_NEGATIVO : return "MSP_TO_CPU_TAMANIO_NEGATIVO";
	case MSP_TO_CPU_SEGMENTO_EXCEDE_TAMANIO_MAXIMO : return "MSP_TO_CPU_SEGMENTO_EXCEDE_TAMANIO_MAXIMO";
	case MSP_TO_CPU_PID_EXCEDE_CANT_MAXIMA_DE_SEGMENTOS : return "MSP_TO_CPU_PID_EXCEDE_CANT_MAXIMA_DE_SEGMENTOS";
	case MSP_TO_CPU_SEGMENTO_CREADO : return "MSP_TO_CPU_SEGMENTO_CREADO";
	case MSP_TO_CPU_SEGMENTO_DESTRUIDO : return "MSP_TO_CPU_SEGMENTO_DESTRUIDO";
	case MSP_TO_KERNEL_DIRECCION_INVALIDA : return "MSP_TO_KERNEL_DIRECCION_INVALIDA";
	case MSP_TO_KERNEL_MEMORIA_INSUFICIENTE : return "MSP_TO_KERNEL_MEMORIA_INSUFICIENTE";
	case MSP_TO_KERNEL_PID_INVALIDO : return "MSP_TO_KERNEL_PID_INVALIDO";
	case MSP_TO_KERNEL_SEGMENTO_DESTRUIDO : return "MSP_TO_KERNEL_SEGMENTO_DESTRUIDO";
	case MSP_TO_KERNEL_SEGMENTO_EXCEDE_TAMANIO_MAXIMO : return "MSP_TO_KERNEL_SEGMENTO_EXCEDE_TAMANIO_MAXIMO";
	case MSP_TO_KERNEL_PID_EXCEDE_CANT_MAXIMA_DE_SEGMENTOS : return "MSP_TO_KERNEL_PID_EXCEDE_CANT_MAXIMA_DE_SEGMENTOS";
	case MSP_TO_KERNEL_TAMANIO_NEGATIVO : return "MSP_TO_KERNEL_TAMANIO_NEGATIVO";
	case MSP_TO_KERNEL_VIOLACION_DE_SEGMENTO : return "MSP_TO_KERNEL_VIOLACION_DE_SEGMENTO";
	case MSP_TO_KERNEL_MEMORIA_ESCRITA : return "MSP_TO_KERNEL_MEMORIA_ESCRITA";
	case MSP_TO_KERNEL_ESPERO_BYTES : return "MSP_TO_KERNEL_ESPERO_BYTES";
	case MSP_TO_CPU_ENVIO_BYTES : return "MSP_TO_CPU_ENVIO_BYTES";
	case CPU_TO_MSP_ESCRIBIR_MEMORIA_NUMERO : return " CPU_TO_MSP_ESCRIBIR_MEMORIA_NUMERO";
	case CPU_TO_MSP_SOLICITAR_NUMERO : return "CPU_TO_MSP_SOLICITAR_NUMERO";

		default:  return "---DEFAULT--- (mensaje sin definir)";
	}
	return "";

}


// **** FUNCIONES AUXILIARES ****
char* separarLineas(char* linea){
	int i;
	for(i=0; linea[i] != '\0'; i++){
		if( linea[i] == '\n' ){
			linea[i] = '~';
		}
	}
	return linea;
}

/**
 * @NAME: imprimir_en_impresora
 * @DESC: Imprime en impresora (Solicitado por el Kernel)
 * @Parameters:
 * 		numSocket = socket destinatario;
 *
 * 		authorizedHeader = el header que confirma la recepcion de código! ***Tiene que ser el mismo header que usa
 * 							quien envia el mensaje. (validación)***
 *
 * 		stringCode = código a recibir, por referencia para recibir el contenido!;
 * 		logger = log de quien consume esta funcionalidad.
 */
char* recibirCodigo(int32_t numSocket, t_header authorizedHeader, t_log *logger) {

	t_contenido mensaje;
	memset(mensaje, 0, sizeof(t_contenido));
	char* stringCode = string_new();

	strcpy(mensaje, "Operacion de recepción de código AnSISOP autorizada!...");
	enviarMensaje(numSocket, authorizedHeader, mensaje, logger);

	memset(mensaje, 0, sizeof(t_contenido));
	t_header response = recibirMensaje(numSocket, mensaje, logger);
	enviarMensaje(numSocket, CODE_HAY_MAS_LINEAS_OK, "", logger);

	while(response != CODE_FIN_LINEAS){
		string_append(&stringCode, mensaje);
		string_append(&stringCode, "\n");
		memset(mensaje, 0, sizeof(t_contenido));
		response = recibirMensaje(numSocket, mensaje, logger);
		enviarMensaje(numSocket, CODE_HAY_MAS_LINEAS_OK, "", logger);
	}

	return stringCode;
}

/**
 * @NAME: imprimir_en_impresora
 * @DESC: Imprime en impresora (Solicitado por el Kernel)
 * @Parameters:
 * 		numSocket = socket destinatario;
 * 		header = el header de cada enviarMensaje presente en la iteracion;
 * 		initialMessage = acá podes poner algún dato que quieras agregar al momento de avisarle al
* 						 destinatario, que le estas por enviar código.
 * 		stringCode = código a enviar;
 * 		logger = log de quien consume esta funcionalidad.
 */
int32_t enviarCodigo(int32_t numSocket, t_header header, t_contenido initialMessage, char* stringCode, t_log *logger, bool NoBrNoComment) {

	t_contenido mensaje;
	
	/*"Aviso al kernel que le voy a pasar el codigo AnSISOP y espero el ok para comenzar transaccion!"*/
	memset(mensaje, 0, sizeof(t_contenido));
	strcpy(mensaje, initialMessage);
	
	enviarMensaje(numSocket, header, mensaje, logger);
	t_header kernelOkResponse = -1;
	
	while(kernelOkResponse != header)
		kernelOkResponse = recibirMensaje(numSocket, mensaje, logger);

	//Me reemplaza los \n por \0 para poder utilizarlo como token e iterar envios.
	separarLineas(stringCode);

	char* buffer;		//Guardo el texto a medida que voy leyendo
	int32_t position = 0;	//Posicion
	int32_t offset = 0;		//Desplazamiento dentro de mi buffer (seek)

	int32_t tamanioTotal = strlen(stringCode);
	char* test = string_new();

	while( position<tamanioTotal ){

		buffer = stringCode+position;

		if(buffer[offset] == '~') {
			memset(mensaje, 0, sizeof(t_contenido));
			strcpy(mensaje, string_substring(stringCode, position, offset));

			test = string_duplicate(mensaje);
			string_trim(&test);

			if(!NoBrNoComment || (!string_equals_ignore_case(string_substring(test, 0, 1), "#") && !string_equals_ignore_case(string_substring(test, 0, 1), ""))){
				enviarMensaje(numSocket, CODE_HAY_MAS_LINEAS, mensaje, logger);
				recibirMensaje(numSocket, mensaje, logger);
			}

			position += (offset + 1);
			offset = -1;
		}
		offset += 1 ;
	}

	memset(mensaje, 0, sizeof(t_contenido));
	strcpy(mensaje, "Code Message transaction completed without errors! :)");
	enviarMensaje(numSocket, CODE_FIN_LINEAS, mensaje, logger);


	return 1;
}


//FUNCIONES QUE USA MARIANO

int enviar(int sock, char *buffer, int tamano)
{
	int escritos;

	if ((escritos = send (sock, buffer, tamano, 0)) <= 0)
	{
		printf("Error en el send\n\n");
		return WARNING;
	}
	else if (escritos != tamano)
	{
		printf("La cantidad de bytes enviados es distinta de la que se quiere enviar\n\n");
		return WARNING;
	}

	return EXITO;
}

int recibir(int sock, char *buffer, int tamano)
{
	int val;
	int leidos = 0;

	memset(buffer, '\0', tamano);

	while (leidos < tamano)
	{

		val = recv(sock, buffer + leidos, tamano - leidos, 0);
		leidos += val;
		if (val < 0)
		{
			printf("Error al recibir datos: %d - %s\n", val, strerror(val)); //ENOTCONN ENOTSOCK
			switch(val) {
				// The  socket  is  marked non-blocking and the receive operation would block, or a receive timeout had been set and the timeout expired before data was received.  POSIX.1-2001 allows either error to be returned for this
				// case, and does not require these constants to have the same value, so a portable application should check for both possibilities.
				//case EAGAIN: printf(" - EAGAIN \n The  socket  is  marked non-blocking and the receive operation would block, or a receive timeout had been set and the timeout expired before data was received.\n\n"); break;
				//case EWOULDBLOCK: printf("EWOULDBLOCK \n The  socket  is  marked non-blocking and the receive operation would block, or a receive timeout had been set and the timeout expired before data was received.\n\n"); break;
				// The argument sockfd is an invalid descriptor.
				case EBADF: printf("EBADF \n The argument sockfd is an invalid descriptor.\n\n"); break;
				// A remote host refused to allow the network connection (typically because it is not running the requested service).
				case ECONNREFUSED: printf("ECONNREFUSED \n A remote host refused to allow the network connection (typically because it is not running the requested service).\n\n"); break;
				// The receive buffer pointer(s) point outside the process's address space.
				case EFAULT: printf("EFAULT \n The receive buffer pointer(s) point outside the process's address space.\n\n"); break;
				// The receive was interrupted by delivery of a signal before any data were available; see signal(7).
				case EINTR: printf("EINTR \n The receive was interrupted by delivery of a signal before any data were available; see signal(7).\n\n"); break;
				// Invalid argument passed.
				case EINVAL: printf("EINVAL \n Invalid argument passed.\n\n"); break;
				// Could not allocate memory for recvmsg().
				case ENOMEM: printf("ENOMEM \n Could not allocate memory for recvmsg().\n\n"); break;
				// The socket is associated with a connection-oriented protocol and has not been connected (see connect(2) and accept(2)).
				case ENOTCONN: printf("ENOTCONN \n The socket is associated with a connection-oriented protocol and has not been connected (see connect(2) and accept(2)).\n\n"); break;
				// The argument sockfd does not refer to a socket.
				case ENOTSOCK: printf("ENOTSOCK \n The argument sockfd does not refer to a socket.\n\n"); break;

			}
			return ERROR;
		}
		// Cuando recv devuelve 0 es porque se desconecto el socket.
		if(val == 0)
		{
			/*printf("%d se desconecto\n", sock);*/
			return WARNING;
		}
	}
	return EXITO;
}

