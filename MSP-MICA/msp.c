/*
 * msp.c
 *
 *  Created on: 26/09/2014
 *      Author: utnso
 */
#include "msp.h"

void levantarArchivoDeConfiguracion()
{
 	int puerto;
	int tamanio;
	int swap;
	char* algoritmo;

	t_config *config;

	config = config_create("configuracion");

	//Compruebo si el archivo tiene todos los datos
	bool estaPuerto = config_has_property(config, "PUERTO");
	bool estaCantMemoria = config_has_property(config, "CANTIDAD_MEMORIA");
	bool estaCantSwap = config_has_property(config, "CANTIDAD_SWAP");
	bool estaSust = config_has_property(config, "SUST_PAGS");

	if(!estaPuerto || !estaCantMemoria || !estaCantSwap || !estaSust)
	{
		if (consola == 1) printf("Error en el archivo de configuración. Ruta: %s\n", config->path);
		log_error(logs, "Error en el archivo de configuración. Ruta: %s.", config->path);
		abort();
	}

	log_info(logs, "La ruta del archivo de configuración es: %s.", config->path);


	algoritmo = strdup(config_get_string_value(config, "SUST_PAGS"));
	puerto = config_get_int_value(config, "PUERTO");
	tamanio = config_get_int_value(config, "CANTIDAD_MEMORIA");
	swap = config_get_int_value(config, "CANTIDAD_SWAP");

	if ((strcmp(algoritmo, "FIFO")!= 0) && (strcmp(algoritmo, "CLOCKM") != 0))
	{
		if (consola == 1) printf("El algoritmo de sustitución de páginas no es válido.\n");
		log_error(logs, "El algoritmo de sustitución de páginas no es válido.");
		abort();
	}

	if ((tamanio < 0) || (swap < 0))
	{
		if (consola == 1) printf("Error, el tamaño de la memoria o el swap son menores a 0.\n");
		log_error(logs, "Error, el tamaño de la memoria o el swap son menores a 0.");
		abort();

	}

	configuracion.sust_pags = algoritmo;
	configuracion.puerto = puerto;
	configuracion.cantidad_memoria = tamanio;
	configuracion.cantidad_swap = swap;

	config_destroy(config);
}

void crearTablaDeMarcos()
{
	if (configuracion.cantidad_memoria % TAMANIO_PAGINA == 0)
	{
		cantidadMarcos = configuracion.cantidad_memoria / TAMANIO_PAGINA;
	}
	else
	{
		//Si la división no es exacta, necesito un marco más para alojar lo que queda de la memoria.
		cantidadMarcos = configuracion.cantidad_memoria / TAMANIO_PAGINA + 1;
	}

	//printf("cantidadmarcos: %d\n", cantidadMarcos);

	//Aloco espacio de memoria para una tabla que va tener toda la información de los marcos de memoria.
	//calloc recibe dos parámetros por separado, la cantidad de espacios que necesito, y el tamaño de cada
	//uno de esos espacios, y además, settea cada espacio con el caracter nulo.
	tablaMarcos = calloc(cantidadMarcos, sizeof(t_marco));

	if (tablaMarcos == NULL)
	{
		log_error(logs, "ERROR: no se pudo crear la tabla de marcos.");
		abort();
	}

	int i;
	for (i=0; i<cantidadMarcos; i++)
	{
		tablaMarcos[i].nro_marco = i;

		tablaMarcos[i].dirFisica = memoriaPrincipal + i * TAMANIO_PAGINA;

		//Cuando recién se crea la tabla, todos los marcos están libres.
		tablaMarcos[i].libre = 1;

		pthread_rwlock_init(&(tablaMarcos[i].rwMarco), NULL);
	}

}

void listarMarcos()
{
	log_info(logs, "TABLA DE MARCOS");
	int i;
	for (i=0; i<cantidadMarcos; i++)
	{
		pthread_rwlock_rdlock(&(tablaMarcos[i].rwMarco));
		log_info(logs, "Numero marco: %d     PID: %d     Numero segmento: %d     Numero pagina: %d	   Libre: %d     Orden: %d      %.256s", tablaMarcos[i].nro_marco, tablaMarcos[i].pid, tablaMarcos[i].nro_segmento, tablaMarcos[i].nro_pagina, tablaMarcos[i].libre, tablaMarcos[i].orden, (char*)tablaMarcos[i].dirFisica);
		if (consola == 1)
		{
			printf("Numero marco: %d     PID: %d     Numero segmento: %d     Numero pagina: %d     ", tablaMarcos[i].nro_marco, tablaMarcos[i].pid, tablaMarcos[i].nro_segmento, tablaMarcos[i].nro_pagina);
			printf("Orden: %d    ", tablaMarcos[i].orden);
			printf("Libre: %d    ", tablaMarcos[i].libre);
			printf("Referencia: %d     ", tablaMarcos[i].referencia);
			printf("Modificación: %d     ", tablaMarcos[i].modificacion);
			printf("%.10s\n", (char*)tablaMarcos[i].dirFisica);
		}
		pthread_rwlock_unlock(&(tablaMarcos[i].rwMarco));


	}
}

void inicializarMSP()
{
	pthread_mutex_init(&mutexMemoriaTotalRestante, NULL);
	pthread_mutex_init(&mutexMPRestante, NULL);
	pthread_mutex_init(&mutexSwapRestante, NULL);
	pthread_mutex_init(&mutexOrdenMarco, NULL);
	pthread_mutex_init(&mutexPuntero, NULL);

	pthread_rwlock_init(&rwListaSegmentos, NULL);

	FILE *log = fopen("logMSP", "w");
	fflush(log);
	fclose(log);

	logs = log_create("logMSP", "MSP", 0, LOG_LEVEL_TRACE);

	levantarArchivoDeConfiguracion();

	//PASAJE DE MB Y KB A BYTES
	int memoriaEnBytes = configuracion.cantidad_memoria * (pow(2, 10));
	int swapEnBytes = configuracion.cantidad_swap * (pow(2, 20));
	configuracion.cantidad_memoria = memoriaEnBytes;
	configuracion.cantidad_swap = swapEnBytes;

	//Estas variables las uso para, cada vez que asigno un marco o swappeo una pagina, voy restando del
	//espacio total. Cuando alguna de estas dos variables llegue a 0, significa que no hay mas espacio.
	memoriaRestante = configuracion.cantidad_memoria;
	swapRestante = configuracion.cantidad_swap;
	tamanioRestanteTotal = swapRestante + memoriaRestante;

	memoriaPrincipal = calloc(configuracion.cantidad_memoria, 1);

	if(memoriaPrincipal == NULL)
	{
		if (consola == 1) printf("Error, no se pudo alocar espacio para la memoria principal.\n");
		log_error(logs, "Error, no se pudo alocar espacio para la memoria principal.");
		abort();
	}

	crearTablaDeMarcos();

	listaSegmentos = list_create();

	consola = 0;

	ordenMarco = 0;

	log_trace(logs, "MSP inicio su ejecución. Tamaño memoria: %d. Tamaño swap: %d. Tamaño total: %d. Algoritmo sust páginas: %s.", memoriaRestante, swapRestante, memoriaRestante+swapRestante, configuracion.sust_pags);

	if(pthread_create(&hilo_consola_1, NULL, (void*)consola_msp, NULL)!=0){
		puts("No se ha podido crear el proceso consola de la MSP.");
	}

}

uint32_t agregarSegmentoALista(int cantidadDePaginas, int pid, int cantidadSegmentosDeEstePid, int tamanio)
{
	nodo_segmento *nodoSegmento;
	nodoSegmento = malloc(sizeof(nodo_segmento));


	t_list *listaSegmentosDelPID = filtrarListaSegmentosPorPid(pid);

	t_list *listaPaginas;
	listaPaginas = crearListaPaginas(cantidadDePaginas);

	nodo_segmento *ultimoNodo;
	if(cantidadSegmentosDeEstePid == 0)
	{
		nodoSegmento->numeroSegmento = 0;
	}

	else
	{
		ultimoNodo = list_get(listaSegmentosDelPID, cantidadSegmentosDeEstePid - 1);
		nodoSegmento->numeroSegmento = (ultimoNodo->numeroSegmento)+1;
	}

	nodoSegmento->pid = pid;
	nodoSegmento->listaPaginas = listaPaginas;
	nodoSegmento->tamanio = tamanio;

	list_add(listaSegmentos, nodoSegmento);


	//Esta es una impresion de prueba.
/*	puts("CREACION DE SEGMENTO");
	printf("Nro seg: %d    PID: %d    Tamanio: %d\n", nodoSegmento->numeroSegmento, nodoSegmento->pid, nodoSegmento->tamanio);
	int i;
	for(i=0; i<cantidadDePaginas; i++)
	{
		nodo_paginas *nodoPagina;
		nodoPagina = list_get(listaPaginas, i);
		uint32_t direccion = generarDireccionLogica(nodoSegmento->numeroSegmento, nodoPagina->nro_pagina, 0);

		printf("		 Nro pag: %d      presencia: %d       direccion: %zu\n",  nodoPagina->nro_pagina, nodoPagina->presencia, direccion);

	}*/
	//Acá termina la impresion de prueba.

	uint32_t direccionBase = generarDireccionLogica(nodoSegmento->numeroSegmento, 0, 0);

	list_destroy(listaSegmentosDelPID);



	return direccionBase;

}

uint32_t crearSegmento(int pid, int tamanio)
{
	if (tamanio < 0)
	{
		log_error(logs, "Error, el tamaño es negativo.");

		if (consola == 1) printf("Error, el tamaño es negativo.\n");
		return TAMANIO_NEGATIVO;
	}

	int cantidadTotalDePaginas;

	pthread_rwlock_wrlock(&rwListaSegmentos);


	t_list *listaDeSegmentosDeEstePid = filtrarListaSegmentosPorPid(pid);
	int cantidadSegmentosDeEstePid = list_size(listaDeSegmentosDeEstePid);
	if (cantidadSegmentosDeEstePid == CANTIDAD_MAX_SEGMENTOS_POR_PID)
	{
		pthread_rwlock_unlock(&rwListaSegmentos);
		log_error(logs, "Error, ya no se pueden agregar más segmentos para PID %d. La cantidad máxima de segmentos por PID es de %d.", pid, CANTIDAD_MAX_SEGMENTOS_POR_PID);
		if (consola == 1) printf("Error, ya no se pueden agregar más segmentos para PID %d. La cantidad máxima de segmentos por PID es de %d.\n", pid, CANTIDAD_MAX_SEGMENTOS_POR_PID);
		return PID_EXCEDE_CANT_MAX_SEGMENTO;
	}

	//Si el resto de la división entre el tamaño y TAMANIO_PAGINA (que es el tamaño máximo de la pagina) da 0...
	if (tamanio % TAMANIO_PAGINA == 0)
	{
		//...entonces la cantiadad de páginas es el resultado de la división, si no...
		cantidadTotalDePaginas = tamanio / TAMANIO_PAGINA;
	}
	else
	{
		//...la cantidad total de páginas es el resultado de la división más 1, para agregar en una página lo que
		//queda del tamaño, por supuesto, esta última página no estará completa.
		cantidadTotalDePaginas = tamanio / TAMANIO_PAGINA + 1;
	}


	pthread_mutex_lock(&mutexMemoriaTotalRestante);
	if (tamanio > tamanioRestanteTotal)
	{
		pthread_rwlock_unlock(&rwListaSegmentos);
		if (consola == 1) printf("No hay espacio en la memoria para crear este segmento.");
		log_error(logs, "No hay espacio disponible en la memoria para crear este segmento.");
		pthread_mutex_unlock(&mutexMemoriaTotalRestante);
		return MEMORIA_INSUFICIENTE;
	}


	tamanioRestanteTotal = tamanioRestanteTotal - (TAMANIO_PAGINA * cantidadTotalDePaginas);
	//pthread_mutex_unlock(&mutexMemoriaTotalRestante);



	if (cantidadTotalDePaginas > CANTIDAD_MAX_PAGINAS_POR_SEGMENTO)
	{
		pthread_rwlock_unlock(&rwListaSegmentos);

		pthread_mutex_unlock(&mutexMemoriaTotalRestante);

		log_error(logs, "Error, no se puede crear el segmento para el PID %d porque excede el tamaño máximo de %d cantidad de páginas.", pid, CANTIDAD_MAX_PAGINAS_POR_SEGMENTO);
		if (consola == 1) printf("Error, no se puede crear el segmento para el PID %d porque excede el tamaño máximo de %d cantidad de páginas.\n", pid, CANTIDAD_MAX_PAGINAS_POR_SEGMENTO);
		return SEGMENTO_EXCEDE_TAM_MAX;
	}


	uint32_t direccionBase = agregarSegmentoALista(cantidadTotalDePaginas, pid, cantidadSegmentosDeEstePid, tamanio);

	int numeroSegmento, numeroPagina, offset;
	obtenerUbicacionLogica(direccionBase, &numeroSegmento, &numeroPagina, &offset);

	int tamanioReal = cantidadTotalDePaginas * TAMANIO_PAGINA;

	log_trace(logs, "Para el PID %d se creó el número de segmento %d de tamanio %d. El tamanio que ocupa en la memoria es de: %d.", pid, numeroSegmento, tamanio, tamanioReal);
	if (consola == 1) printf("Para el PID %d se creó el número de segmento %d de tamanio %d. El tamanio que ocupa en la memoria es de: %d.\n", pid, numeroSegmento, tamanio, tamanioReal);
	log_trace(logs, "El espacio restante en la memoria es de %d bytes.", tamanioRestanteTotal);
	if (consola == 1) printf("El espacio restante en la memoria es de %d bytes.", tamanioRestanteTotal);

	pthread_mutex_unlock(&mutexMemoriaTotalRestante);
	pthread_rwlock_unlock(&rwListaSegmentos);


	list_destroy(listaDeSegmentosDeEstePid);

	return direccionBase;

}

int destruirSegmento(int pid, uint32_t base)
{
	int numeroSegmento, numeroPagina, offset;

	obtenerUbicacionLogica(base, &numeroSegmento, &numeroPagina, &offset);

	if (numeroPagina != 0 || offset != 0)
	{
		log_error(logs, "Error, la dirección proporcionada no es una dirección base.");
		if (consola == 1) printf("Error, la dirección proporcionada no es una dirección base.\n");
		return DIRECCION_INVALIDA;
	}

	bool _pidYSegmentoCorresponde(nodo_segmento *p) {
		return (p->pid == pid && p->numeroSegmento == numeroSegmento);
	}

	bool _pidCorresponde(nodo_segmento *p) {
		return (p->pid == pid);
	}

	bool _segmentoCorresponde (nodo_segmento *p)
	{
		return (p->numeroSegmento == numeroSegmento);
	}

	nodo_segmento *nodo;

	//puts("hola");

	pthread_rwlock_wrlock(&rwListaSegmentos);

	//puts("chau");
/*	if (!list_any_satisfy(listaSegmentos, (void*)_pidYSegmentoCorresponde))
	{
		pthread_rwlock_unlock(&rwListaSegmentos);
		log_error(logs, "Error, PID y/o segmento inválidos.");
		if (consola == 1) printf("Error, pid y/o segmento invalidos\n");
		return PID_INEXISTENTE;
	}*/

	if (!list_any_satisfy(listaSegmentos, (void*)_pidCorresponde))
	{
		pthread_rwlock_unlock(&rwListaSegmentos);
		log_error(logs, "Error, PID inválido.");
		if (consola == 1) printf("Error, PID inválido.\n");
		return PID_INEXISTENTE;
	}

	if (!list_any_satisfy(listaSegmentos, (void*)_segmentoCorresponde))
	{
		//printf ("Segmento numero: %d\n", numeroSegmento);
		pthread_rwlock_unlock(&rwListaSegmentos);
		log_error(logs, "Error, segmento inválido.");
		if (consola == 1) printf("Error, segmento inválido.\n");
		return DIRECCION_INVALIDA;
	}

	nodo = list_remove_by_condition(listaSegmentos, (void*)_pidYSegmentoCorresponde);


	void _liberarMarcoOBorrarArchivoSwap(nodo_paginas *nodoPagina)
	{
		pthread_rwlock_wrlock(&(nodoPagina->rwPagina));
		if (nodoPagina->presencia == -2)
		{
			//char* nombreArchivo = malloc(60);

			char* nombreArchivo;

			nombreArchivo = generarNombreArchivo(nodo->pid, nodo->numeroSegmento, nodoPagina->nro_pagina);

			FILE *archivo = fopen(nombreArchivo, "w");
			remove(nombreArchivo);
			fflush(archivo);
			fclose(archivo);

			swapRestante = swapRestante + TAMANIO_PAGINA;

			log_trace(logs, "Se eliminó el archivo %s debido a la destrucción de la página %d del segmento %d del PID %d.", nombreArchivo, nodoPagina->nro_pagina, nodo->numeroSegmento, pid);
			if (consola == 1) printf("Se eliminó el archivo %s debido a la destrucción de la página %d del segmento %d del PID %d.\n", nombreArchivo, nodoPagina->nro_pagina, nodo->numeroSegmento, pid);

			free(nombreArchivo);
		}

		if ((nodoPagina->presencia >= 0))
		{
			int numeroMarco = nodoPagina->presencia;

			pthread_rwlock_wrlock(&(tablaMarcos[numeroMarco].rwMarco));

			liberarMarco(numeroMarco, nodoPagina);

			pthread_rwlock_unlock(&(tablaMarcos[numeroMarco].rwMarco));


			log_trace(logs, "Se liberó el marco n° %d debido a la destrucción de la página %d del segmento %d del PID %d.\n", numeroMarco, nodoPagina->nro_pagina, nodo->numeroSegmento, pid);
			if (consola == 1) printf("Se liberó el marco n° %d debido a la destrucción de la página %d del segmento %d del PID %d.\n", numeroMarco, nodoPagina->nro_pagina, nodo->numeroSegmento, pid);
		}

		pthread_rwlock_unlock(&(nodoPagina->rwPagina));


		pthread_rwlock_destroy(&(nodoPagina->rwPagina));

		free(nodoPagina);
	}

	list_iterate(nodo->listaPaginas, (void*)_liberarMarcoOBorrarArchivoSwap);

	int cantidadPaginas = list_size(nodo->listaPaginas);

	list_destroy(nodo->listaPaginas);

	pthread_mutex_lock(&mutexMemoriaTotalRestante);
	tamanioRestanteTotal = tamanioRestanteTotal + cantidadPaginas * TAMANIO_PAGINA;
	//printf("Cantiddad de paginas: %d\n", cantidadPaginas);
	int tamanioSegmento = nodo->tamanio;

	log_trace(logs, "Se destruyó el segmento %d del PID %d, cuyo tamanio era de %d.", nodo->numeroSegmento, pid, tamanioSegmento);
	if (consola == 1) printf("Se destruyó el segmento %d del PID %d, cuyo tamanio era de %d.\n", nodo->numeroSegmento, pid, tamanioSegmento);
	log_trace(logs, "El espacio restante en la memoria es de %d bytes.", tamanioRestanteTotal);
	if (consola == 1) printf("El espacio restante en la memoria es de %d bytes.", tamanioRestanteTotal);

	pthread_mutex_unlock(&mutexMemoriaTotalRestante);


	//free(nodo->listaPaginas);

	free(nodo);

	pthread_rwlock_unlock(&rwListaSegmentos);



	return DIRECCION_VALIDA;
}

t_list* crearListaPaginas(int cantidadDePaginas)
{
	t_list* listaPaginas;

	listaPaginas = list_create();

	int i;

	for (i = 0; i<cantidadDePaginas; i++)
	{
		nodo_paginas* nodoPagina = malloc(sizeof(nodo_paginas));
		nodoPagina->nro_pagina = i;
		nodoPagina->presencia = -1;
		pthread_rwlock_init(&(nodoPagina->rwPagina), NULL);

		list_add(listaPaginas, nodoPagina);
	}

	return listaPaginas;
}

void tablaSegmentos()
{

	printf("TABLA DE SEGMENTOS\n");
	log_info(logs, "TABLA DE SEGMENTOS");

	pthread_rwlock_rdlock(&rwListaSegmentos);

	if (list_is_empty(listaSegmentos))
	{
		pthread_rwlock_unlock(&rwListaSegmentos);

		printf("No hay segmentos creados en el sistema.\n");
		log_info(logs, "No hay segmentos creados en el sistema.\n");
		return;
	}

	void _imprimirSegmento(nodo_segmento *nodoSegmento)
	{
		int cantidadPaginas = list_size(nodoSegmento->listaPaginas);
		uint32_t direccion = generarDireccionLogica(nodoSegmento->numeroSegmento, 0, 0);
		int tamanioSegmento = TAMANIO_PAGINA * cantidadPaginas;
		printf("PID: %d\tN°Segmento: %d\tTamaño: %d\tTamanio real: %d\tDirección base: %d\n", nodoSegmento->pid, nodoSegmento->numeroSegmento, nodoSegmento->tamanio, tamanioSegmento, direccion);
		log_info(logs, "PID: %d\tN°Segmento: %d\tTamaño: %d\tDirección base:%d", nodoSegmento->pid, nodoSegmento->numeroSegmento, nodoSegmento->tamanio, tamanioSegmento, direccion);
	}

	list_iterate(listaSegmentos, (void*)_imprimirSegmento);

	pthread_rwlock_unlock(&rwListaSegmentos);
}

void tablaPaginas(int pid)
{
	pthread_rwlock_rdlock(&rwListaSegmentos);

	t_list *listaFiltrada = filtrarListaSegmentosPorPid(pid);

	if(list_is_empty(listaFiltrada))
	{
		pthread_rwlock_unlock(&rwListaSegmentos);
		log_error(logs, "No se encontró el pid solicitado");
		printf("Error, no se encontro el pid solicitado.\n");
		return;
	}

	log_info(logs, "TABLA DE PÁGINAS DEL PID %d.", pid);
	printf("TABLA DE PÁGINAS DEL PID %d.\n", pid);
	void imprimirDatosSegmento(nodo_segmento *nodoSegmento)
	{
		if (list_is_empty(nodoSegmento->listaPaginas))
		{
			log_info(logs, "PID: %d\tNro segmento: %d\tNo tiene páginas.\n", nodoSegmento->pid, nodoSegmento->numeroSegmento);
			printf("PID: %d\tNro segmento: %d\tNo tiene páginas.\n", nodoSegmento->pid, nodoSegmento->numeroSegmento);
		}

		else
		{
			void imprimirDatosPaginas(nodo_paginas *nodoPagina)
			{
				pthread_rwlock_rdlock(&(nodoPagina->rwPagina));
				printf("PID: %d\tNro segmento: %d\tNro Pagina: %d\tPresencia: %d\n", nodoSegmento->pid, nodoSegmento->numeroSegmento, nodoPagina->nro_pagina, nodoPagina->presencia);
				log_info(logs, "Nro segmento: %d\tPID: %d\tNro Pagina: %d\tPresencia: %d", nodoSegmento->numeroSegmento, nodoSegmento->pid, nodoPagina->nro_pagina, nodoPagina->presencia);
				pthread_rwlock_unlock(&(nodoPagina->rwPagina));

			}

			list_iterate(nodoSegmento->listaPaginas, (void*)imprimirDatosPaginas);
		}


	}
	list_iterate(listaFiltrada, (void*)imprimirDatosSegmento);

	pthread_rwlock_unlock(&rwListaSegmentos);

	list_destroy(listaFiltrada);

}

uint32_t generarDireccionLogica(int numeroSegmento, int numeroPagina, int offset)
{
	uint32_t direccion = numeroSegmento;
	direccion = direccion << 12;
	direccion = direccion | numeroPagina;
	direccion = direccion << 8;
	direccion = direccion | offset;

	return direccion;
}

void obtenerUbicacionLogica(uint32_t direccion, int *numeroSegmento, int *numeroPagina, int *offset)
{
	*offset = direccion & 0xFF;
	*numeroPagina = (direccion >> 8) & 0xFFF;
	*numeroSegmento = (direccion >> 20) & 0xFFF;
}

t_list* filtrarListaSegmentosPorPid(int pid)
{
	bool _pidCorresponde(nodo_segmento *p) {
		return (p->pid == pid);
	}

	t_list *listaFiltrada;


	listaFiltrada = list_filter(listaSegmentos, (void*)_pidCorresponde);


	return listaFiltrada;
}

int direccionInValida(uint32_t direccionLogica, int pid, int tamanio)
{

	t_list* listaFiltradaPorPID = filtrarListaSegmentosPorPid(pid);

	if (list_is_empty(listaFiltradaPorPID))
	{
		list_destroy(listaFiltradaPorPID);
		return PID_INEXISTENTE;
	}


	int numeroSegmento, numeroPagina, offset;
	obtenerUbicacionLogica(direccionLogica, &numeroSegmento, &numeroPagina, &offset);

	nodo_segmento *nodoSegmento;
	bool _segmentoCorresponde(nodo_segmento *p){
		return(p->numeroSegmento == numeroSegmento);
	}
	nodoSegmento = list_find(listaFiltradaPorPID, (void*)_segmentoCorresponde);

	if (nodoSegmento == NULL)
	{
		list_destroy(listaFiltradaPorPID);

		return DIRECCION_INVALIDA;
	}

	if (offset > nodoSegmento->tamanio)
	{
		list_destroy(listaFiltradaPorPID);

		return VIOLACION_DE_SEGMENTO;
	}

	if (offset > (TAMANIO_PAGINA - 1))
	{
		list_destroy(listaFiltradaPorPID);

		return DIRECCION_INVALIDA;
	}

	if (nodoSegmento->tamanio == 0 && tamanio > 0)
	{
		list_destroy(listaFiltradaPorPID);

		return VIOLACION_DE_SEGMENTO;
	}

	t_list* listaPaginas = nodoSegmento->listaPaginas;

	nodo_paginas *nodoPagina;
	bool _paginaCorresponde(nodo_paginas *p){

		return(p->nro_pagina == numeroPagina);
	}
	nodoPagina = list_find(listaPaginas, (void*)_paginaCorresponde);

	if (nodoPagina == NULL)
	{
		if (nodoSegmento->tamanio == 0)
		{
			list_destroy(listaFiltradaPorPID);

			return DIRECCION_VALIDA;
		}
		list_destroy(listaFiltradaPorPID);


		return DIRECCION_INVALIDA;
	}

	if (nodoPagina->nro_pagina == 0)
	{
		if ((tamanio + offset) > nodoSegmento->tamanio)
		{
			list_destroy(listaFiltradaPorPID);

			return VIOLACION_DE_SEGMENTO;
		}
	}

	int espacioAntesDeLaBase = nodoPagina->nro_pagina *  TAMANIO_PAGINA + offset;
	if ((espacioAntesDeLaBase + tamanio) > nodoSegmento->tamanio)
	{
		list_destroy(listaFiltradaPorPID);

		return VIOLACION_DE_SEGMENTO;
	}

	list_destroy(listaFiltradaPorPID);


	return DIRECCION_VALIDA;
}


nodo_segmento* buscarNumeroSegmento(t_list* listaSegmentosFiltradosPorPID, int numeroSegmento)
{
	bool _numeroCorresponde(nodo_segmento *p) {
		return (p->numeroSegmento == numeroSegmento);
	}

	nodo_segmento *nodo;

	nodo = list_find(listaSegmentosFiltradosPorPID, (void*)_numeroCorresponde);

	return nodo;
}

t_list* paginasQueVoyAUsar(nodo_segmento *nodoSegmento, int numeroPagina, int cantidadPaginas)
{

	///////**********************FIJATE LOS BLOQUEAOOOOOOOOOOOOOOOOOOSSSSSSSSSSSSSSSSSSSSSSS ************
	//////////ÑDSJFSKDJFKDSJFÑKSJFKSLJFLKSJDLKFSJDLKFJFLDKJFDJDSFKJDSKDSKFDKDSFKJKFDSKFDSFDSFDLKDSJFLKDSJFLDS



	t_list* paginasQueNecesito;

	paginasQueNecesito = list_create();

	int i, ultimaPagina;

	ultimaPagina = numeroPagina + cantidadPaginas;

	for (i=numeroPagina; i<ultimaPagina; i++)
	{
		nodo_paginas* nodoPagina;


		nodoPagina = list_get(nodoSegmento->listaPaginas, i);

		list_add(paginasQueNecesito, nodoPagina);

	}


	return paginasQueNecesito;
}

t_list* validarEscrituraOLectura(int pid, uint32_t direccionLogica, int tamanio)
{
	t_list* listaFiltradaPorPid = filtrarListaSegmentosPorPid(pid);

	t_list* paginasQueNecesito;

/*	if (list_is_empty(listaFiltradaPorPid))
	{
		free(listaFiltradaPorPid);
		log_error(logs, "Error, el PID ingresado no existe.");
		if (consola == 1) printf("Error, el pid ingresado no existe.\n");
		return NULL;
	}

	int numeroSegmento, numeroPagina, offset;
	obtenerUbicacionLogica(direccionLogica, &numeroSegmento, &numeroPagina, &offset);
	if (offset > TAMANIO_PAGINA - 1)
	{
		list_destroy(listaFiltradaPorPid);
		//printf("numeroSegmento: %d   pid: %d   pag: %d   direccion: %d", numeroSegmento, pid, numeroPagina, (int)direccionLogica);
		log_error(logs, "Error, la dirección ingresada es inválida.");
		if (consola == 1) printf("Error, la dirección ingresada es inválida.\n");
		return NULL;
	}

	//Ahora de la lista de los segmentos correspondientes a este pid, quiero el segmento
	//que me dice la dirección lógica. La función find me devuelve el primer resultado que encuentra,
	//pero en este caso eso está bien porque hay un sólo segmento de número numeroSegmento en el espacio de
	//direcciones del proceso pid.
	nodo_segmento *nodoSegmento;
	bool _segmentoCorresponde(nodo_segmento *p){
		return(p->numeroSegmento == numeroSegmento);
	}
	nodoSegmento = list_find(listaFiltradaPorPid, (void*)_segmentoCorresponde);

	if (nodoSegmento->tamanio == 0 && tamanio > 0)
	{
		list_destroy(listaFiltradaPorPid);
		log_error(logs, "Error, violación de segmento.");
		if (consola == 1) printf("Error, violacion de segmento.\n");
		return NULL;
	}

	if (nodoSegmento == NULL)
	{
		list_destroy(listaFiltradaPorPid);
		log_error(logs, "Error, la dirección ingresada es inválida.");
		if (consola == 1) printf("Error, la dirección ingresada es inválida.\n");
		return NULL;
	}

	t_list* listaPaginas = nodoSegmento->listaPaginas;

	nodo_paginas *nodoPagina;
	bool _paginaCorresponde(nodo_paginas *p){
		return(p->nro_pagina == numeroPagina);
	}
	nodoPagina = list_find(listaPaginas, (void*)_paginaCorresponde);

	if (direccionInValida(direccionLogica, listaFiltradaPorPid))
	{
		list_destroy(listaFiltradaPorPid);
		//printf("numeroSegmento: %d   pid: %d   pag: %d   direccion: %d", numeroSegmento, pid, numeroPagina, (int)direccionLogica);
		log_error(logs, "Error, la dirección ingresada es inválida.");
		if (consola == 1) printf("Error, la dirección ingresada es inválida.\n");
		return NULL;
	}

	if (nodoPagina == NULL && nodoSegmento->tamanio == 0)
	{
		list_destroy(listaFiltradaPorPid);
		paginasQueNecesito = paginasQueVoyAUsar(nodoSegmento, 0, 0);
		return (paginasQueNecesito);
	}*/

	int direccionInvalida = direccionInValida(direccionLogica, pid, tamanio);

	switch (direccionInvalida)
	{
		case DIRECCION_INVALIDA:
		{
			list_destroy(listaFiltradaPorPid);
			//printf("numeroSegmento: %d   pid: %d   pag: %d   direccion: %d", numeroSegmento, pid, numeroPagina, (int)direccionLogica);
			log_error(logs, "Error, la dirección ingresada es inválida.");
			if (consola == 1) printf("Error, la dirección ingresada es inválida.\n");
			return NULL;
		}
		case VIOLACION_DE_SEGMENTO:
		{
			list_destroy(listaFiltradaPorPid);
			log_error(logs, "Error, violación de segmento.");
			if (consola == 1) printf("Error, violacion de segmento.\n");
			return NULL;
		}
		case PID_INEXISTENTE:
		{
			free(listaFiltradaPorPid);
			log_error(logs, "Error, el PID ingresado no existe.");
			if (consola == 1) printf("Error, el pid ingresado no existe.\n");
			return NULL;
		}

	}

	int numeroSegmento, numeroPagina, offset;
	obtenerUbicacionLogica(direccionLogica, &numeroSegmento, &numeroPagina, &offset);

	nodo_segmento *nodoSegmento;
	bool _segmentoCorresponde(nodo_segmento *p){
		return(p->numeroSegmento == numeroSegmento);
	}
	nodoSegmento = list_find(listaFiltradaPorPid, (void*)_segmentoCorresponde);
	t_list *listaPaginas = nodoSegmento->listaPaginas;

	nodo_paginas *nodoPagina;
	bool _paginaCorresponde(nodo_paginas *p){
		return(p->nro_pagina == numeroPagina);
	}
	nodoPagina = list_find(listaPaginas, (void*)_paginaCorresponde);


	if (nodoPagina == NULL && nodoSegmento->tamanio == 0)
	{
		list_destroy(listaFiltradaPorPid);
		paginasQueNecesito = paginasQueVoyAUsar(nodoSegmento, 0, 0);
		return (paginasQueNecesito);
	}

/*	if (nodoPagina == NULL)
	{
		//printf("numeroSegmento: %d   pid: %d   pag: %d   direccion: %d", numeroSegmento, pid, numeroPagina, (int)direccionLogica);
		log_error(logs, "Error, la dirección ingresada es inválida.");
		if (consola == 1) printf("Error, la dirección ingresada es inválida.\n");
		return NULL;
	}*/

	//COMPROBACION DE VIOLACION DE SEGMENTO Esta variable me indica los bytes que hay antes de la direccion base
/*	int espacioAntesDeLaBase = nodoPagina->nro_pagina *  TAMANIO_PAGINA;
	if ((espacioAntesDeLaBase + tamanio) > nodoSegmento->tamanio)
	{
		list_destroy(listaFiltradaPorPid);
		log_error(logs, "Error, violación de segmento.");
		if (consola == 1) printf("Error, violacion de segmento.\n");
		return NULL;
	}*/

	//Tengo que transformar el tamanio en paginas, para ver si el segmneto tiene esa cantidad de paginas
	int cantidadPaginasQueOcupaTamanio;
	int faltaParaCompletarPagina = TAMANIO_PAGINA - offset;
	int quedaDelTamanio = tamanio - faltaParaCompletarPagina;
	if (quedaDelTamanio < 0)
	{
		cantidadPaginasQueOcupaTamanio = 1;
		paginasQueNecesito = paginasQueVoyAUsar(nodoSegmento, nodoPagina->nro_pagina, cantidadPaginasQueOcupaTamanio);
	}
	else
	{
		int cantidadPaginasEnterasQueQuedan = quedaDelTamanio / TAMANIO_PAGINA;
		//Me fijo si necesito una pagina para alojar el remanente
		if (quedaDelTamanio % TAMANIO_PAGINA != 0)
		{
			cantidadPaginasQueOcupaTamanio = cantidadPaginasEnterasQueQuedan + 1;
		}
		else
		{
			cantidadPaginasQueOcupaTamanio = cantidadPaginasEnterasQueQuedan;
		}
		//Ahora, cantidadPaginasQueOcupaTamanio representa, a partir de la pagina dedse donde empiezo a escribir (y sin contarla) las páginas que
		//tendría que tener el segmento para que tamanio entre
/*
		int cantidadPaginasSegmento = list_size(listaPaginas);
		if ((nodoPagina->nro_pagina + cantidadPaginasQueOcupaTamanio) > (cantidadPaginasSegmento -1))
		{
			log_error(logs, "Error, violación de segmento.");
			if (consola == 1) printf("Error, violacion de segmento.\n");
			return NULL;
		}
*/



		paginasQueNecesito = paginasQueVoyAUsar(nodoSegmento, nodoPagina->nro_pagina, cantidadPaginasQueOcupaTamanio+1);
	}


	list_destroy(listaFiltradaPorPid);

	return paginasQueNecesito;

}

void* solicitarMemoria(int pid, uint32_t direccionLogica, int tamanio)
{
	void* buffermini; //Este es el buffer auxiliar que uso para copiar lo que
						//está en cada cachito de memoria. Despues lo muevo al buffer definitivo, es una cuestion de orden de los cachos.
	int numeroSegmento, numeroPagina, offset;

	//puts("hola");
	pthread_rwlock_trywrlock(&rwListaSegmentos);
	//puts("chau");

	t_list* paginasQueNecesito = validarEscrituraOLectura(pid, direccionLogica, tamanio);

/*
	void _imprimirNumeroPagina(nodo_paginas *nodoPagina)
	{
		printf("voy a usar pagina: %d\n", nodoPagina->nro_pagina);
	}

	list_iterate(paginasQueNecesito, (void*)_imprimirNumeroPagina);*/

	if (paginasQueNecesito == NULL)
	{
		pthread_rwlock_unlock(&(rwListaSegmentos));
		free(paginasQueNecesito);
		return NULL;
	}
	else if(list_is_empty(paginasQueNecesito))
	{
		pthread_rwlock_unlock(&(rwListaSegmentos));
		free(paginasQueNecesito);
		return NULL;
	}

	obtenerUbicacionLogica(direccionLogica, &numeroSegmento, &numeroPagina, &offset);

	void _traerAMemoriaPaginasSwappeadasParaLeer(nodo_paginas *nodo)
	{
		pthread_rwlock_wrlock(&(nodo->rwPagina));
		if (nodo->presencia == -2)
		{
			moverPaginaDeSwapAMemoria(pid, numeroSegmento, nodo);
		}
		pthread_rwlock_unlock(&(nodo->rwPagina));
	}

	list_iterate(paginasQueNecesito, (void*)_traerAMemoriaPaginasSwappeadasParaLeer);

	void _traerAMemoriaPaginasSinMarco(nodo_paginas *nodo)
	{
		pthread_rwlock_wrlock(&(nodo->rwPagina));

		if (nodo->presencia == -1)
		{
			pthread_mutex_lock(&mutexMPRestante);
			if(memoriaRestante < TAMANIO_PAGINA)
			{
				if (consola == 1) printf("No hay espacio en la memoria principal, hay que swappear.");
				log_trace(logs, "No hay espacio en la memoria principal, hay que swappear.");

				if (strcmp(configuracion.sust_pags, "FIFO") == 0)
				{
					elegirVictimaSegunFIFO();
				}
				else
				{
					elegirVictimaSegunClockM();
				}
			}

			pthread_mutex_unlock(&mutexMPRestante);

			buscarYAsignarMarcoLibre(pid, numeroSegmento, nodo);
		}

		pthread_rwlock_unlock(&(nodo->rwPagina));


	}

	list_iterate(paginasQueNecesito, (void*)_traerAMemoriaPaginasSinMarco);


	void* buffer = malloc(tamanio+1);
	//memset(buffer, 0, tamanio + 1);

	//Tomo el primer nodo de la lista de paginas que necesito


	nodo_paginas *nodoPagina = list_get(paginasQueNecesito, 0);



	int yaCopie = TAMANIO_PAGINA - offset;
	void* direccionOrigen;

	//Si no se le asigno ningun marco a esa pagina (nunca se usó esa página) pongo vacio ese espacio en el buffer
/*	if (nodoPagina->presencia == -1)
	{
		buffermini = malloc(yaCopie);
		memset (buffermini, 0, yaCopie);
		memcpy (buffer, buffermini, yaCopie);
		free(buffermini);
	}*/
	pthread_rwlock_rdlock(&(nodoPagina->rwPagina));

	if (yaCopie > tamanio)
	{


		direccionOrigen = offset + tablaMarcos[nodoPagina->presencia].dirFisica;
		memcpy (buffer, direccionOrigen, tamanio);
		pthread_rwlock_wrlock(&(tablaMarcos[nodoPagina->presencia].rwMarco));

		tablaMarcos[nodoPagina->presencia].referencia = 1;
		pthread_rwlock_unlock(&(tablaMarcos[nodoPagina->presencia].rwMarco));

		//printf("buffffffffffffffer: %s\n", (char*)buffer);
		list_destroy(paginasQueNecesito);

		pthread_rwlock_unlock(&(nodoPagina->rwPagina));


		pthread_rwlock_unlock(&rwListaSegmentos);


		return buffer;

	}
	else
	{

		direccionOrigen = offset + tablaMarcos[nodoPagina->presencia].dirFisica;
		memcpy(buffer, direccionOrigen, yaCopie);
		//puts("buffffer:1");
		//printf("ya copie: %d\n", yaCopie);
		//puts((char*)buffer);
		pthread_rwlock_wrlock(&(tablaMarcos[nodoPagina->presencia].rwMarco));

		tablaMarcos[nodoPagina->presencia].referencia = 1;
		pthread_rwlock_unlock(&(tablaMarcos[nodoPagina->presencia].rwMarco));

		pthread_rwlock_unlock(&(nodoPagina->rwPagina));




	}



	//printf("cant pag que necd: %d\n", list_size(paginasQueNecesito));

	//pthread_rwlock_unlock(&(nodoPagina->rwPagina));

	//copio las paginas del medio
	int i;
	for (i=1; i<((list_size(paginasQueNecesito)) - 1); i++)
	{
		nodoPagina = list_get(paginasQueNecesito, i);
		pthread_rwlock_rdlock(&(nodoPagina->rwPagina));

		direccionOrigen = tablaMarcos[nodoPagina->presencia].dirFisica;
		buffermini = malloc(TAMANIO_PAGINA);
		memset(buffermini, 0, TAMANIO_PAGINA);
		memcpy(buffermini, direccionOrigen, TAMANIO_PAGINA);

		memcpy(buffer + yaCopie, buffermini, TAMANIO_PAGINA);
		//puts("buffffer:2");
		//puts((char*)buffer);
		yaCopie = yaCopie + TAMANIO_PAGINA;
		free(buffermini);
		pthread_rwlock_wrlock(&(tablaMarcos[nodoPagina->presencia].rwMarco));
		tablaMarcos[nodoPagina->presencia].referencia = 1;
		pthread_rwlock_unlock(&(tablaMarcos[nodoPagina->presencia].rwMarco));

		pthread_rwlock_unlock(&(nodoPagina->rwPagina));

	}

	//copio lo que me queda del tamanio de la ultima pagina
	if ((list_size(paginasQueNecesito) > 1))
	{
		nodoPagina = list_get(paginasQueNecesito, i);
		pthread_rwlock_rdlock(&(nodoPagina->rwPagina));
		direccionOrigen = tablaMarcos[nodoPagina->presencia].dirFisica;
		buffermini = malloc(tamanio - yaCopie);
		memset(buffermini, 0, tamanio - yaCopie);
		memcpy(buffermini, direccionOrigen, tamanio - yaCopie);
		memcpy(buffer + yaCopie, buffermini, tamanio - yaCopie);
		//puts("buffffer:3");
		//puts((char*)buffer);
		free(buffermini);
		pthread_rwlock_wrlock(&(tablaMarcos[nodoPagina->presencia].rwMarco));

		tablaMarcos[nodoPagina->presencia].referencia = 1;
		pthread_rwlock_unlock(&(tablaMarcos[nodoPagina->presencia].rwMarco));

		pthread_rwlock_unlock(&(nodoPagina->rwPagina));

	}

	pthread_rwlock_unlock(&rwListaSegmentos);

	//puts("hola desbloquie la lista de seg");



	list_destroy(paginasQueNecesito);

	return buffer;
}

void moverPaginaDeSwapAMemoria(int pid, int segmento, nodo_paginas* nodoPagina)
{

	char* nombreArchivo = malloc(60);

	int pagina = nodoPagina->nro_pagina;

	nombreArchivo = generarNombreArchivo(pid, segmento, pagina);

	FILE *archivo = fopen(nombreArchivo, "r");


	if (memoriaRestante < TAMANIO_PAGINA)
	{
		if (strcmp(configuracion.sust_pags, "FIFO") == 0)
		{
			elegirVictimaSegunFIFO();
		}
		else
		{
			elegirVictimaSegunClockM();
		}
	}

	void* buffer = malloc(TAMANIO_PAGINA);

	fread(buffer, 1, TAMANIO_PAGINA, archivo);

	remove(nombreArchivo);

	fclose(archivo);

	pthread_mutex_lock(&mutexSwapRestante);
	swapRestante = swapRestante + TAMANIO_PAGINA;
	log_trace(logs, "Se movió a memoria principal la página %d del segmento %d del PID %d. Swap restante: %d.", pagina, segmento, pid, swapRestante);
	pthread_mutex_unlock(&mutexSwapRestante);


	buscarYAsignarMarcoLibre(pid, segmento, nodoPagina);

	int numeroMarco = nodoPagina->presencia;

	pthread_rwlock_wrlock(&(tablaMarcos[numeroMarco].rwMarco));

	escribirEnMarco(numeroMarco, TAMANIO_PAGINA, buffer, 0, 0);

	pthread_rwlock_wrlock(&(tablaMarcos[numeroMarco].rwMarco));



	free(buffer);
	free(nombreArchivo);
}

int escribirMemoria(int pid, uint32_t direccionLogica, void* bytesAEscribir, int tamanio)
{
	pthread_rwlock_trywrlock(&rwListaSegmentos);

	t_list* paginasQueNecesito = validarEscrituraOLectura(pid, direccionLogica, tamanio);

	if (paginasQueNecesito == NULL)
	{
		pthread_rwlock_unlock(&rwListaSegmentos);

		free(paginasQueNecesito);
		return EXIT_FAILURE;
	}
	else if(list_is_empty(paginasQueNecesito))
	{
		pthread_rwlock_unlock(&rwListaSegmentos);

		puts("entre");
		free(paginasQueNecesito);
		return EXIT_SUCCESS;
	}

	int numeroSegmento, numeroPagina, offset;
	obtenerUbicacionLogica(direccionLogica, &numeroSegmento, &numeroPagina, &offset);

	int cantidadPaginasQueNecesito = list_size(paginasQueNecesito);

	int yaEscribi = 0;
	int i;
	int quedaParaCompletarPagina = TAMANIO_PAGINA - offset;
	int	tamanioRestante = tamanio - quedaParaCompletarPagina;


	for (i=0; i<cantidadPaginasQueNecesito; i++)
	{

		nodo_paginas* nodoPagina = list_get(paginasQueNecesito, i);
		pthread_rwlock_wrlock(&(nodoPagina->rwPagina));


		if (nodoPagina->presencia == -1)
		{
			pthread_mutex_lock(&mutexMPRestante);
			if(memoriaRestante < TAMANIO_PAGINA)
			{
				if (consola == 1) printf("No hay espacio en la memoria principal, hay que swappear.");
				log_trace(logs, "No hay espacio en la memoria principal, hay que swappear.");

				if (strcmp(configuracion.sust_pags, "FIFO") == 0)
				{
					elegirVictimaSegunFIFO();
				}
				else
				{
					elegirVictimaSegunClockM();
				}
			}


			pthread_mutex_lock(&mutexPuntero);

			buscarYAsignarMarcoLibre(pid, numeroSegmento, nodoPagina);
			pthread_mutex_unlock(&mutexPuntero);


			pthread_mutex_unlock(&mutexMPRestante);

		}

		if (nodoPagina->presencia == -2)
		{
			pthread_mutex_lock(&mutexOrdenMarco);
			moverPaginaDeSwapAMemoria(pid, numeroSegmento, nodoPagina);
			ordenMarco = ordenMarco - 1; //Esto lo hago para que no se saltee ningun orden. Cuando traigo la pagina a memoria,
			pthread_mutex_unlock(&mutexOrdenMarco);
							//hago un escribirEnMarco que aumenta el orden de escritura, y cuando escribo en nuevo buffer
										//tambien, por lo que el orden se aumenta dos veces.
		}

		pthread_rwlock_wrlock(&(tablaMarcos[nodoPagina->presencia].rwMarco));


		if (i == 0)
		{

			if(tamanio <= quedaParaCompletarPagina)
			{
				//pthread_rwlock_wrlock(&(tablaMarcos[nodoPagina->presencia].rwMarco));
				pthread_mutex_lock(&mutexOrdenMarco);
				pthread_mutex_lock(&mutexPuntero);


				escribirEnMarco (nodoPagina->presencia, tamanio, bytesAEscribir, offset, 0);
				pthread_mutex_unlock(&mutexPuntero);

				pthread_mutex_unlock(&mutexOrdenMarco);

				tablaMarcos[nodoPagina->presencia].modificacion = 1;
				tablaMarcos[nodoPagina->presencia].referencia = 1;
				//pthread_rwlock_unlock(&(tablaMarcos[nodoPagina->presencia].rwMarco));

			}
			else
			{
				yaEscribi = yaEscribi + quedaParaCompletarPagina;
				tamanioRestante = tamanio - quedaParaCompletarPagina;
				//pthread_rwlock_wrlock(&(tablaMarcos[nodoPagina->presencia].rwMarco));
				pthread_mutex_lock(&mutexOrdenMarco);
				pthread_mutex_lock(&mutexPuntero);


				escribirEnMarco (nodoPagina->presencia, quedaParaCompletarPagina, bytesAEscribir, offset, 0);
				pthread_mutex_unlock(&mutexPuntero);

				pthread_mutex_unlock(&mutexOrdenMarco);

				tablaMarcos[nodoPagina->presencia].modificacion = 1;
				tablaMarcos[nodoPagina->presencia].referencia = 1;
				//pthread_rwlock_unlock(&(tablaMarcos[nodoPagina->presencia].rwMarco));


			}

		}

		if ((tamanioRestante / TAMANIO_PAGINA == 0) && (i!=0))
		{
			//pthread_rwlock_wrlock(&(tablaMarcos[nodoPagina->presencia].rwMarco));
			pthread_mutex_lock(&mutexOrdenMarco);
			pthread_mutex_lock(&mutexPuntero);


			escribirEnMarco (nodoPagina->presencia, tamanioRestante, bytesAEscribir, 0, yaEscribi);
			pthread_mutex_unlock(&mutexPuntero);

			pthread_mutex_unlock(&mutexOrdenMarco);

			tablaMarcos[nodoPagina->presencia].modificacion = 1;
			tablaMarcos[nodoPagina->presencia].referencia = 1;
			//pthread_rwlock_unlock(&(tablaMarcos[nodoPagina->presencia].rwMarco));

			tamanioRestante = tamanioRestante - (tamanioRestante % TAMANIO_PAGINA);
			yaEscribi = yaEscribi + (tamanioRestante % TAMANIO_PAGINA);

		}
		else if ((tamanioRestante / TAMANIO_PAGINA > 0) && (i!=0))
		{
			//pthread_rwlock_wrlock(&(tablaMarcos[nodoPagina->presencia].rwMarco));
			pthread_mutex_lock(&mutexOrdenMarco);
			pthread_mutex_lock(&mutexPuntero);


			escribirEnMarco (nodoPagina->presencia, TAMANIO_PAGINA, bytesAEscribir, 0, yaEscribi);
			pthread_mutex_unlock(&mutexPuntero);

			pthread_mutex_unlock(&mutexOrdenMarco);

			tablaMarcos[nodoPagina->presencia].modificacion = 1;
			tablaMarcos[nodoPagina->presencia].referencia = 1;

			tamanioRestante = tamanioRestante - TAMANIO_PAGINA;
			yaEscribi = yaEscribi + TAMANIO_PAGINA;

		}
		pthread_rwlock_unlock(&(tablaMarcos[nodoPagina->presencia].rwMarco));

		pthread_rwlock_unlock(&(nodoPagina->rwPagina));

	}



	list_destroy(paginasQueNecesito);


	pthread_rwlock_unlock(&rwListaSegmentos);


	return EXIT_SUCCESS;

}

void escribirEnMarco(int numeroMarco, int tamanio, void* bytesAEscribir, int offset, int yaEscribi)
{
	void* direccionDestino = tablaMarcos[numeroMarco].dirFisica + offset;

	ordenMarco ++;

	tablaMarcos[numeroMarco].orden = ordenMarco;

	memcpy(direccionDestino, bytesAEscribir + yaEscribi, tamanio);

}

void* buscarYAsignarMarcoLibre(int pid, int numeroSegmento, nodo_paginas *nodoPagina)
{
	int i;
	for (i = 0; i<cantidadMarcos; i++)
	{
		pthread_rwlock_wrlock(&(tablaMarcos[i].rwMarco));
		//Uso el primer marco libre que encuentro
		if (tablaMarcos[i].libre == 1)
		{
			if ((i+1) == cantidadMarcos)
			{
				puntero = 0;
			}
			else
			{
				puntero = i+1;
			}

			memoriaRestante = memoriaRestante - TAMANIO_PAGINA;


			tablaMarcos[i].libre = 0;
			tablaMarcos[i].nro_pagina = nodoPagina->nro_pagina;
			tablaMarcos[i].nro_segmento = numeroSegmento;
			tablaMarcos[i].pid = pid;
			nodoPagina->presencia = i;

			log_trace(logs, "Se asignó el marco %d a la página %d del segmento %d del PID %d.", i, nodoPagina->nro_pagina, numeroSegmento, pid);

			pthread_rwlock_unlock(&(tablaMarcos[i].rwMarco));


			return (tablaMarcos[i].dirFisica);
		}

		pthread_rwlock_unlock(&(tablaMarcos[i].rwMarco));


	}


	return NULL;
}

void conexionConKernelYCPU()
{

	struct sockaddr_in my_addr, their_addr;

	int socketFD, newFD;

	socketFD = crearSocket();

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(configuracion.puerto);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', sizeof(struct sockaddr_in));

	bindearSocket(socketFD, my_addr);

	escucharEn(socketFD);

	while(1)
	{
		socklen_t sin_size;

		sin_size = sizeof(struct sockaddr_in);


		log_trace(logs, "A la espera de nuevas conexiones");

		if((newFD = accept(socketFD,(struct sockaddr *)&their_addr, &sin_size)) == -1)
		{
			perror("accept");
			continue;
		}

		log_trace(logs, "Recibí conexion de %s", inet_ntoa(their_addr.sin_addr));

		t_contenido mensajeParaRecibirConexionCpu;
		memset (mensajeParaRecibirConexionCpu, 0, sizeof(t_contenido));

		t_header header_conexion_MSP = recibirMensaje(newFD, mensajeParaRecibirConexionCpu, logs);

		pthread_t hilo;

		if(header_conexion_MSP == CPU_TO_MSP_HANDSHAKE)
		{

			log_info(logs,"Entre a conexion con cpu");
			pthread_create(&hilo, NULL, atenderACPU, (void*)newFD);

		}

		if(header_conexion_MSP == KERNEL_TO_MSP_HANDSHAKE){

			log_info(logs,"Entre a conexion con kernel");
			pthread_create(&hilo, NULL, atenderAKernel, (void*)newFD);
		}

		//pthread_join(hilo, NULL);

	}


	pthread_exit(0);
}


/*
 * esta funcion la toca MARIANO Y LEANDRO
 *
 */

/*
 * esta funcion la toca MARIANO Y LEANDRO
 *
 */

void* atenderAKernel(void* socket_kernel)
{
	log_info(logs, "Se creo el hilo para atender al kernel, espero peticiones del kernel:");
	bool running = true;

	while(running){
		t_contenido mensaje;
		memset(mensaje, 0, sizeof(t_contenido));
		t_header headerK = recibirMensaje((int)socket_kernel, mensaje, logs);

		if(headerK == ERR_CONEXION_CERRADA){
			log_error(logs, "el Hilo que atiende el KERNEL se desconecto, "
					"por lo que no se puede continuar");
			running = false;
			terminarMSP();
			exit(0);
			//break;

		} else if(headerK == KERNEL_TO_MSP_ELIMINAR_SEGMENTOS){

			char** split = string_get_string_as_array(mensaje);
			int pid = atoi(split[0]);
			uint32_t base = atoi(split[1]);
			int exito = EXIT_FAILURE;

			exito = destruirSegmento(pid, base);


			switch (exito)
			{
				case DIRECCION_VALIDA:
				{
					t_contenido mensaje_direccion;
					memset(mensaje_direccion, 0,sizeof(t_contenido));
					enviarMensaje((int)socket_kernel, MSP_TO_KERNEL_SEGMENTO_DESTRUIDO,mensaje_direccion, logs);
					break;
				}
				case DIRECCION_INVALIDA:
				{
					t_contenido mensaje_direccion;
					memset(mensaje_direccion, 0,sizeof(t_contenido));
					enviarMensaje((int)socket_kernel,MSP_TO_KERNEL_DIRECCION_INVALIDA,mensaje_direccion, logs);
					break;

				}
				case PID_INEXISTENTE:
				{
					t_contenido mensaje_direccion;
					memset(mensaje_direccion, 0,sizeof(t_contenido));
					enviarMensaje((int)socket_kernel,MSP_TO_KERNEL_PID_INVALIDO,mensaje_direccion, logs);
					break;

				}
			}


			//log_info(logs,"ELIMINAR SEGMENTO :%d",pid);


			//enviarMensaje((int)socket_kernel, KERNEL_TO_MSP_ELIMINAR_SEGMENTOS, string_from_format("%d", exito), logs);
		} else if(headerK == KERNEL_TO_MSP_MEM_REQ){

			char** split = string_get_string_as_array(mensaje);
			int pid = atoi(split[0]);
			int tamanio = atoi(split[1]);
			int exito = EXIT_FAILURE;

			exito = crearSegmento(pid, tamanio);

			switch (exito)
			{
				case MEMORIA_INSUFICIENTE:
				{
					t_contenido mensaje_direccion;
					memset(mensaje_direccion, 0,sizeof(t_contenido));
					enviarMensaje((int)socket_kernel,MSP_TO_KERNEL_MEMORIA_INSUFICIENTE, mensaje_direccion, logs);
					break;

				}
				case TAMANIO_NEGATIVO:
				{
					t_contenido mensaje_direccion;
					memset(mensaje_direccion, 0,sizeof(t_contenido));
					enviarMensaje((int)socket_kernel,MSP_TO_KERNEL_TAMANIO_NEGATIVO,mensaje_direccion, logs);
					break;

				}
				case PID_EXCEDE_CANT_MAX_SEGMENTO:
				{
					t_contenido mensaje_direccion;
					memset(mensaje_direccion, 0,sizeof(t_contenido));
					enviarMensaje((int)socket_kernel,MSP_TO_KERNEL_PID_EXCEDE_CANT_MAXIMA_DE_SEGMENTOS,mensaje_direccion, logs);
					break;

				}
				case SEGMENTO_EXCEDE_TAM_MAX:
				{
					t_contenido mensaje_direccion;
					memset(mensaje_direccion, 0,sizeof(t_contenido));
					enviarMensaje((int)socket_kernel,MSP_TO_KERNEL_SEGMENTO_EXCEDE_TAMANIO_MAXIMO,mensaje_direccion, logs);
					break;

				}
				default:
				{
					t_contenido mensaje_direccion;
					memset(mensaje_direccion, 0,sizeof(t_contenido));
					enviarMensaje((int)socket_kernel, MSP_TO_KERNEL_SEGMENTO_CREADO, string_from_format("%d", exito), logs);
				}
			}


			log_info(logs, "Ya puede seguir, se creó el segmento.");



		} else if(headerK == KERNEL_TO_MSP_ENVIAR_BYTES){
			char** split = string_get_string_as_array(mensaje);
			char *buffer = NULL;
			int pid = atoi(split[0]);
			int tamanio = atoi(split[1]);
			uint32_t direccion = atoi(split[2]);
			//int exito;


			enviarMensaje((int)socket_kernel, MSP_TO_KERNEL_ESPERO_BYTES, string_from_format("Ahora esperamos el buffer de escritura del proceso %d", pid), logs);

			buffer = calloc(tamanio, 1);

			recibir((int)socket_kernel, buffer, tamanio);

			pthread_rwlock_wrlock(&rwListaSegmentos);


			int error = mandarErrorkernel((int)socket_kernel, direccion, pid, tamanio);

			if (error == EXIT_SUCCESS)
			{
				escribirMemoria(pid, direccion, buffer, tamanio);
				log_info(logs, "Ya puede seguir, se escribio memoria.");
				t_contenido mensaje_instruccion;
				memset(mensaje_instruccion,0,sizeof(t_contenido));
				enviarMensaje((int)socket_kernel,MSP_TO_KERNEL_MEMORIA_ESCRITA,mensaje_instruccion,logs);
			}
			else
			{
				pthread_rwlock_unlock(&rwListaSegmentos);
			}



			free(buffer);

		}

	}


	return EXIT_SUCCESS;

}


int mandarErrorkernel (int socket_kernel, int dirLogica, int pid, int tamanio)
{
	int i = EXIT_SUCCESS;
	switch (direccionInValida(dirLogica, pid, tamanio))
	{
		case VIOLACION_DE_SEGMENTO:
		{
			//pthread_rwlock_unlock(&rwListaSegmentos);
			t_contenido mensaje_instruccion;
			memset(mensaje_instruccion,0,sizeof(t_contenido));
			enviarMensaje(socket_kernel,MSP_TO_KERNEL_VIOLACION_DE_SEGMENTO,mensaje_instruccion,logs);
			i = EXIT_FAILURE;
			break;
		}
		case PID_INEXISTENTE:
		{
			//pthread_rwlock_unlock(&rwListaSegmentos);

			t_contenido mensaje_instruccion;
			memset(mensaje_instruccion,0,sizeof(t_contenido));
			enviarMensaje(socket_kernel,MSP_TO_KERNEL_PID_INVALIDO,mensaje_instruccion,logs);
			i = EXIT_FAILURE;
			break;

		}
		case DIRECCION_INVALIDA:
		{
			//pthread_rwlock_unlock(&rwListaSegmentos);

			t_contenido mensaje_instruccion;
			memset(mensaje_instruccion,0,sizeof(t_contenido));
			enviarMensaje(socket_kernel,MSP_TO_KERNEL_DIRECCION_INVALIDA,mensaje_instruccion,logs);
			i = EXIT_FAILURE;
			break;

		}
	}

	return i;

}


int mandarErrorCPU (int socket_cpu, int dirLogica, int pid, int tamanio)
{
	int i = EXIT_SUCCESS;
	switch (direccionInValida(dirLogica, pid, tamanio))
	{
		case VIOLACION_DE_SEGMENTO:
		{
			//pthread_rwlock_unlock(&rwListaSegmentos);

			t_contenido mensaje_instruccion;
			memset(mensaje_instruccion,0,sizeof(t_contenido));
			enviarMensaje(socket_cpu,MSP_TO_CPU_VIOLACION_DE_SEGMENTO,mensaje_instruccion,logs);
			i = EXIT_FAILURE;
			break;
		}
		case PID_INEXISTENTE:
		{
			//pthread_rwlock_unlock(&rwListaSegmentos);

			t_contenido mensaje_instruccion;
			memset(mensaje_instruccion,0,sizeof(t_contenido));
			enviarMensaje(socket_cpu,MSP_TO_CPU_PID_INVALIDO,mensaje_instruccion,logs);
			i = EXIT_FAILURE;
			break;

		}
		case DIRECCION_INVALIDA:
		{
			//pthread_rwlock_unlock(&rwListaSegmentos);

			t_contenido mensaje_instruccion;
			memset(mensaje_instruccion,0,sizeof(t_contenido));
			enviarMensaje(socket_cpu,MSP_TO_CPU_DIRECCION_INVALIDA,mensaje_instruccion,logs);
			i = EXIT_FAILURE;
			break;
		}


	}

	return i;

}


void* atenderACPU(void* socket_cpu)
{
	bool socketValidador = true;

	while(socketValidador){

			int pid;

			t_contenido mensaje;
			memset(mensaje,0,sizeof(t_contenido));
			t_header header_recibido = recibirMensaje((int)socket_cpu, mensaje, logs);

			if(header_recibido == ERR_CONEXION_CERRADA){
						log_error(logs, "Termino conexión con CPU porque se cerró la conexión.");
						socketValidador = false;
						break;
			}

			char** array_aux = string_get_string_as_array(mensaje);

			if(header_recibido == CPU_TO_MSP_SOLICITAR_BYTES){


				//Cargo lo que recibi --> el pir,dir_logica y tamanio


				//log_info(logs,"ENTRO EN EL IF 1");

				char** array_1 = string_get_string_as_array(mensaje);
				pid = atoi(array_1[0]);
				uint32_t dir_logica = atoi(array_1[1]);
				int tamanio = atoi(array_1[2]);

				pthread_rwlock_wrlock(&rwListaSegmentos);

				int error = mandarErrorCPU((int)socket_cpu, dir_logica, pid, tamanio);

				//int error = EXIT_SUCCESS;

				if (error == EXIT_SUCCESS)
				{
					void* buffer_instruccion = solicitarMemoria(pid,dir_logica,tamanio);

					t_contenido mensaje_instruccion;
					memset(mensaje_instruccion,0,sizeof(t_contenido));
					memcpy(mensaje_instruccion,buffer_instruccion,tamanio);
					enviarMensaje((int)socket_cpu,MSP_TO_CPU_BYTES_ENVIADOS,mensaje_instruccion,logs);
					free(buffer_instruccion);
				}
				else
				{
					pthread_rwlock_unlock(&rwListaSegmentos);
				}

			}

			if(header_recibido == CPU_TO_MSP_PARAMETROS){

				char** array = string_get_string_as_array(mensaje);


				int32_t program_counter = atoi(array[0]);
				int32_t auxiliar_cant_bytes = atoi(array[1]);

				pthread_rwlock_wrlock(&rwListaSegmentos);


				int error = mandarErrorCPU((int)socket_cpu, program_counter, pid, auxiliar_cant_bytes);

				if (error == EXIT_SUCCESS)
				{
					void* buffer_parametros = solicitarMemoria(pid,program_counter,auxiliar_cant_bytes);
					t_contenido mensaje_parametros;
					memset(mensaje_parametros,0,sizeof(t_contenido));
					memcpy(mensaje_parametros, buffer_parametros,auxiliar_cant_bytes);
					enviarMensaje((int)socket_cpu,MSP_TO_CPU_BYTES_ENVIADOS,mensaje_parametros,logs);
					free(buffer_parametros);
				}

				else
				{
					pthread_rwlock_unlock(&rwListaSegmentos);
				}

			}


			if(header_recibido == CPU_TO_MSP_CREAR_SEGMENTO ){

				char** array_1 = string_get_string_as_array(mensaje);
				int pid = atoi(array_1[0]);
				int32_t tamanio = atoi(array_1[1]);

				uint32_t direccion = crearSegmento(pid, tamanio);

				switch (direccion)
				{
					case MEMORIA_INSUFICIENTE:
					{
						t_contenido mensaje_direccion;
						memset(mensaje_direccion, 0,sizeof(t_contenido));
						enviarMensaje((int)socket_cpu,MSP_TO_CPU_MEMORIA_INSUFICIENTE,mensaje_direccion, logs);
						break;
					}
					case TAMANIO_NEGATIVO:
					{
						t_contenido mensaje_direccion;
						memset(mensaje_direccion, 0,sizeof(t_contenido));
						enviarMensaje((int)socket_cpu,MSP_TO_CPU_TAMANIO_NEGATIVO,mensaje_direccion, logs);
						break;
					}
					case SEGMENTO_EXCEDE_TAM_MAX:
					{
						t_contenido mensaje_direccion;
						memset(mensaje_direccion, 0,sizeof(t_contenido));
						enviarMensaje((int)socket_cpu,MSP_TO_CPU_SEGMENTO_EXCEDE_TAMANIO_MAXIMO,mensaje_direccion, logs);
						break;
					}
					case PID_EXCEDE_CANT_MAX_SEGMENTO:
					{
						t_contenido mensaje_direccion;
						memset(mensaje_direccion, 0,sizeof(t_contenido));
						enviarMensaje((int)socket_cpu,MSP_TO_CPU_PID_EXCEDE_CANT_MAXIMA_DE_SEGMENTOS,mensaje_direccion, logs);
						break;
					}
					default:
					{
						char* direccion_string = string_itoa(direccion);
						t_contenido mensaje_direccion;
						memset(mensaje_direccion, 0,sizeof(t_contenido)+4);
						memcpy(mensaje_direccion, direccion_string, sizeof(t_contenido));
						enviarMensaje((int)socket_cpu,MSP_TO_CPU_SEGMENTO_CREADO,mensaje_direccion, logs);

					}
				}



			}

			// Es para destruir segmento
			if(header_recibido == CPU_TO_MSP_DESTRUIR_SEGMENTO){

				char** array_1 = string_get_string_as_array(mensaje);
				int pid = atoi(array_1[0]);
				int32_t registro = atoi(array_1[1]);

				int resultado = destruirSegmento(pid, registro);

				switch (resultado)
				{
					case DIRECCION_VALIDA:
					{
						t_contenido mensaje_direccion;
						memset(mensaje_direccion, 0,sizeof(t_contenido));
						enviarMensaje((int)socket_cpu,MSP_TO_CPU_SEGMENTO_DESTRUIDO,mensaje_direccion, logs);
						break;
					}
					case DIRECCION_INVALIDA:
					{
						t_contenido mensaje_direccion;
						memset(mensaje_direccion, 0,sizeof(t_contenido));
						enviarMensaje((int)socket_cpu,MSP_TO_CPU_DIRECCION_INVALIDA,mensaje_direccion, logs);
						break;

					}
					case PID_INEXISTENTE:
					{
						t_contenido mensaje_direccion;
						memset(mensaje_direccion, 0,sizeof(t_contenido));
						enviarMensaje((int)socket_cpu,MSP_TO_CPU_PID_INVALIDO,mensaje_direccion, logs);
						break;

					}
				}

				//log_info(logs,"el segmento fue destruido");



			}

			if(header_recibido == CPU_TO_MSP_ESCRIBIR_MEMORIA){

				char** array = string_get_string_as_array(mensaje);

				int32_t A = atoi(array[0]);
				int32_t B = atoi(array[1]);
				int32_t pid = atoi(array[3]);

				char* buffer = malloc(B);
				memset(buffer,0,sizeof(B));
				memcpy(buffer,(void*)array[2],B); //Ver si hay que castear realmente porque tengo un numero tmb en el push

				pthread_rwlock_wrlock(&rwListaSegmentos);

				int error = mandarErrorCPU((int)socket_cpu, A, pid, B);

				if (error == EXIT_SUCCESS)
				{
					escribirMemoria(pid,A,buffer,B);

					log_info(logs, "Ya puede seguir, se escribio memoria.");
					t_contenido mensaje_instruccion;
					memset(mensaje_instruccion,0,sizeof(t_contenido));
					enviarMensaje((int)socket_cpu,MSP_TO_CPU_ENVIO_BYTES,mensaje_instruccion,logs);
				}
				else
				{
					pthread_rwlock_unlock(&rwListaSegmentos);
				}

				free(buffer);

			}


			if(header_recibido == CPU_TO_MSP_SOLICITAR_NUMERO){


							int32_t	pid1 = atoi(array_aux[0]);
							uint32_t dir_logica1 = atoi(array_aux[1]);
							int tamanio1 = atoi(array_aux[2]);

							pthread_rwlock_wrlock(&rwListaSegmentos);

							int error = mandarErrorCPU((int)socket_cpu, dir_logica1, pid1, tamanio1);

							//int error = EXIT_SUCCESS;

							if (error == EXIT_SUCCESS)
							{
								void* buffer_instruccion = solicitarMemoria(pid1,dir_logica1,tamanio1);

								int32_t auxiliar_numero = 0;
								memcpy(&auxiliar_numero,buffer_instruccion,tamanio1);

								t_contenido mensaje_instruccion;
								memset(mensaje_instruccion,0,sizeof(t_contenido));
								strcpy(mensaje_instruccion, string_from_format("[%d]",auxiliar_numero));
								enviarMensaje((int)socket_cpu,MSP_TO_CPU_BYTES_ENVIADOS,mensaje_instruccion,logs);
								free(buffer_instruccion);
							}
							else
							{
								pthread_rwlock_unlock(&rwListaSegmentos);
							}

						}


			if(header_recibido == CPU_TO_MSP_ESCRIBIR_MEMORIA_NUMERO){



							int32_t A = atoi(array_aux[0]);
							int32_t B = atoi(array_aux[1]);
							int32_t pid = atoi(array_aux[3]);

							char* buffer = malloc(B);
							memset(buffer,0,sizeof(B));

							int32_t numero_auxiliar = atoi(array_aux[2]);

						//	memcpy(buffer,(void*)array[2],B); //Ver si hay que castear realmente porque tengo un numero tmb en el push

							memcpy(buffer,&numero_auxiliar,B);

							pthread_rwlock_wrlock(&rwListaSegmentos);

							int error = mandarErrorCPU((int)socket_cpu, A, pid, B);

							if (error == EXIT_SUCCESS)
							{
								escribirMemoria(pid,A,buffer,B);

								log_info(logs, "Ya puede seguir, se escribio memoria.");
								t_contenido mensaje_instruccion;
								memset(mensaje_instruccion,0,sizeof(t_contenido));
								enviarMensaje((int)socket_cpu,MSP_TO_CPU_ENVIO_BYTES,mensaje_instruccion,logs);
							}
							else
							{
								pthread_rwlock_unlock(&rwListaSegmentos);
							}

							free(buffer);

						}




	}//Fin while

				log_info(logs,"Termino conexion con cpu ------------ Esperando nuevas conexiones");
				return EXIT_SUCCESS;
 }

char* generarNombreArchivo(int pid, int numeroSegmento, int numeroPagina)
{
	char* nombreArchivo = malloc(60);
	char* pidStr = malloc(60);
	char* numeroPaginaStr = malloc(60);
	char* numeroSegmentoStr = malloc(60);

	pidStr = string_itoa(pid);
	numeroPaginaStr = string_itoa(numeroPagina);
	numeroSegmentoStr = string_itoa(numeroSegmento);

	nombreArchivo = string_from_format("%s_%s_%s", pidStr, numeroSegmentoStr, numeroPaginaStr);

	free(pidStr);
	free(numeroPaginaStr);
	free(numeroSegmentoStr);

	return nombreArchivo;
}

int primeraVueltaClock(int puntero)
{
	int numeroMarco = -1;
	int i = puntero;
	int primeraVez = 1;

	while ((i < cantidadMarcos) && (primeraVez == 1))
	{
/*
		puts ("Lista primera vez");
		listarMarcos();
		printf("ii: %d\n", i);*/

		pthread_rwlock_rdlock(&(tablaMarcos[i].rwMarco));


		if ((tablaMarcos[i].referencia == 0) && (tablaMarcos[i].modificacion == 0))
		{

			if ((i+1) == cantidadMarcos)
			{
				puntero = 0;
			}
			else
			{
				puntero = i+1;
			}

			numeroMarco = i;
			//pthread_rwlock_unlock(&(tablaMarcos[i].rwMarco));

			return numeroMarco;
		}


		pthread_rwlock_unlock(&(tablaMarcos[i].rwMarco));


		i++;
		//esto es para fingir la cola circular
		if (i == cantidadMarcos)
		{
			i = 0;
		}

		if (i == puntero)
		{
			primeraVez = 0;
		}

		if ((i+1) == cantidadMarcos)
		{
			puntero = 0;
		}
		else
		{
			puntero = i+1;
		}


	}



	return numeroMarco;
}

int segundaVueltaClock(int puntero)
{
	int numeroMarco = -1;
	int i = puntero;
	int primeraVez = 1;

	while ((i < cantidadMarcos) && (primeraVez == 1))
	{
/*		puts ("Lista segunda");
		listarMarcos();
		printf("ii: %d\n", i);*/

		pthread_rwlock_wrlock(&(tablaMarcos[i].rwMarco));


		if ((tablaMarcos[i].referencia == 0) && (tablaMarcos[i].modificacion == 1))
		{

			if ((i+1) == cantidadMarcos)
			{
				puntero = 0;
			}
			else
			{
				puntero = i+1;
			}
			numeroMarco = i;
			tablaMarcos[i].modificacion = 0;
			//pthread_rwlock_unlock(&(tablaMarcos[i].rwMarco));

			return numeroMarco;
		}

		tablaMarcos[i].referencia = 0;

		pthread_rwlock_unlock(&(tablaMarcos[i].rwMarco));

		i++;
		//esto es para fingir la cola circular
		if (i == cantidadMarcos)
		{
			i = 0;
		}

		if (i == puntero)
		{
			primeraVez = 0;
		}

		if ((i+1) == cantidadMarcos)
		{
			puntero = 0;
		}
		else
		{
			puntero = i+1;
		}



	}



	return numeroMarco;
}


void elegirVictimaSegunClockM()
{
	//int i = 0;
	int numeroMarcoVictima = -1;

	pthread_mutex_lock(&mutexPuntero);


	while (numeroMarcoVictima == -1)
	{
		numeroMarcoVictima = primeraVueltaClock(puntero);

		if (numeroMarcoVictima == -1)
		{
			numeroMarcoVictima = segundaVueltaClock(puntero);
		}
	}

	pthread_mutex_unlock(&mutexPuntero);


	t_marco nodoMarco = tablaMarcos[numeroMarcoVictima];

	//pthread_rwlock_wrlock(&(tablaMarcos[numeroMarcoVictima].rwMarco));


	//printf("numero marco: %d\n", numeroMarcoVictima);



	swappearDeMemoriaADisco(nodoMarco);

	pthread_rwlock_unlock(&(tablaMarcos[numeroMarcoVictima].rwMarco));


}

void swappearDeMemoriaADisco(t_marco nodoMarco)
{
	nodo_segmento *nodoSegmento;
	nodo_paginas *nodoPagina;
	t_list *listaPaginasDelSegmento;
	t_list *listaSegmentosDelPid;


	int numeroSegmento = nodoMarco.nro_segmento;
	int numeroPagina = nodoMarco.nro_pagina;

	listaSegmentosDelPid = filtrarListaSegmentosPorPid(nodoMarco.pid);

	nodoSegmento = buscarNumeroSegmento(listaSegmentosDelPid, numeroSegmento);

	listaPaginasDelSegmento = nodoSegmento->listaPaginas;

	nodoPagina = list_get(listaPaginasDelSegmento, numeroPagina);

	crearArchivoDePaginacion(nodoSegmento->pid, nodoSegmento->numeroSegmento, nodoPagina);

	liberarMarco(nodoMarco.nro_marco, nodoPagina);

	pthread_mutex_lock(&mutexSwapRestante);
	swapRestante = swapRestante - TAMANIO_PAGINA;

	log_trace(logs, "Se desalojó de memoria principal a la página %d del segmento %d del PID %d. El marco liberado es el n° %d. El espacio de swap restante es de: %d.", nodoPagina->nro_pagina, nodoSegmento->numeroSegmento, nodoSegmento->pid, nodoMarco.nro_marco, swapRestante);
	pthread_mutex_unlock(&mutexSwapRestante);

	list_destroy(listaSegmentosDelPid);

}

void elegirVictimaSegunFIFO()
{
	nodo_segmento *nodoSegmento;
	nodo_paginas *nodoPagina;
	t_list *listaPaginasDelSegmento;
	t_list *listaSegmentosDelPid;

	swapRestante = swapRestante - TAMANIO_PAGINA;

	t_marco nodoMarco = tablaMarcos[0];
	int i;
	for (i=1; i<cantidadMarcos; i++)
	{
		pthread_rwlock_wrlock(&(tablaMarcos[i].rwMarco));

		if((tablaMarcos[i].orden < nodoMarco.orden) && (tablaMarcos[i].pid != -1))
		{
			nodoMarco = tablaMarcos[i];
		}

		pthread_rwlock_unlock(&(tablaMarcos[i].rwMarco));

	}
	pthread_rwlock_wrlock(&(nodoMarco.rwMarco));


	int numeroSegmento = nodoMarco.nro_segmento;
	int numeroPagina = nodoMarco.nro_pagina;

	listaSegmentosDelPid = filtrarListaSegmentosPorPid(nodoMarco.pid);

	nodoSegmento = buscarNumeroSegmento(listaSegmentosDelPid, numeroSegmento);

	listaPaginasDelSegmento = nodoSegmento->listaPaginas;

	nodoPagina = list_get(listaPaginasDelSegmento, numeroPagina);

	crearArchivoDePaginacion(nodoSegmento->pid, nodoSegmento->numeroSegmento, nodoPagina);



	liberarMarco(nodoMarco.nro_marco, nodoPagina);

	pthread_rwlock_unlock(&(nodoMarco.rwMarco));


	log_trace(logs, "Se desalojó de memoria principal a la página %d del segmento %d del PID %d. El marco liberado es el n° %d.", nodoPagina->nro_pagina, nodoSegmento->numeroSegmento, nodoSegmento->pid, nodoMarco.nro_marco);

	list_destroy(listaSegmentosDelPid);

}

void liberarMarco(int numeroMarco, nodo_paginas *nodoPagina)
{
	tablaMarcos[numeroMarco].libre = 1;
	tablaMarcos[numeroMarco].nro_pagina = 0;
	tablaMarcos[numeroMarco].nro_segmento = 0;
	tablaMarcos[numeroMarco].orden = 0;
	tablaMarcos[numeroMarco].pid = 0;

	nodoPagina->presencia = -2;

	void* direccionDestino = tablaMarcos[numeroMarco].dirFisica;
	void* buffer = malloc(TAMANIO_PAGINA+1);
	buffer = string_repeat('\0', TAMANIO_PAGINA+1);

	memcpy(direccionDestino, buffer, TAMANIO_PAGINA);

	free(buffer);

	//pthread_mutex_lock(&mutexMPRestante);
	memoriaRestante = memoriaRestante + TAMANIO_PAGINA;
	//pthread_mutex_unlock(&mutexMPRestante);

}

int crearArchivoDePaginacion(int pid, int numeroSegmento, nodo_paginas *nodoPagina)
{
	char* nombreArchivo;
	nombreArchivo = generarNombreArchivo(pid, numeroSegmento, nodoPagina->nro_pagina);

	FILE *archivoPaginacion = fopen(nombreArchivo, "w");

	if (archivoPaginacion == NULL)
	{
		log_error(logs, "No pudo abrirse el archivo de paginacion.");
		return EXIT_FAILURE;
	}

	int numeroMarco = nodoPagina->presencia;

	void* direccionDestino = tablaMarcos[numeroMarco].dirFisica;

	fwrite(direccionDestino, 1, TAMANIO_PAGINA, archivoPaginacion);

	fclose(archivoPaginacion);

	log_trace(logs, "Se creó el archivo de swap %s para alojar a la página %d del segmento %d del PID %d.", nombreArchivo, nodoPagina->nro_pagina, numeroSegmento, pid);

	free(nombreArchivo);


	return EXIT_SUCCESS;

}

uint32_t aumentarProgramCounter(uint32_t programCounterAnterior, int bytesASumar)
{
	uint32_t nuevoProgramCounter;
	int numeroSegmento, numeroPagina, offset;

	obtenerUbicacionLogica(programCounterAnterior, &numeroSegmento, &numeroPagina, &offset);

	if (offset + bytesASumar >= TAMANIO_PAGINA)
	{
		int faltaParaCompletarPagina = TAMANIO_PAGINA - offset ;
		int quedaParaSumar = bytesASumar - faltaParaCompletarPagina;
		int paginaFinal, offsetPaginaFinal;

		paginaFinal = numeroPagina + quedaParaSumar / TAMANIO_PAGINA + 1;
		offsetPaginaFinal = quedaParaSumar % TAMANIO_PAGINA;

		nuevoProgramCounter = generarDireccionLogica(numeroSegmento, paginaFinal, offsetPaginaFinal);

	}

	else
	{
		nuevoProgramCounter = generarDireccionLogica(numeroSegmento, numeroPagina, offset + bytesASumar);
	}


	return nuevoProgramCounter;
}

void terminarMSP()
{
	pthread_mutex_destroy(&mutexMemoriaTotalRestante);
	pthread_mutex_destroy(&mutexMPRestante);
	pthread_mutex_destroy(&mutexSwapRestante);
	pthread_mutex_destroy(&mutexOrdenMarco);
	pthread_mutex_destroy(&mutexPuntero);

	pthread_rwlock_destroy(&rwListaSegmentos);

	//free(memoriaPrincipal);

	//faltan destruir los segmentos


	if (consola == 1) printf("La MSP terminó su ejecución.\n");
	log_info(logs, "La MSP terminó su ejecución.");
	//free(tablaMarcos);
	//log_destroy(logs);



}




