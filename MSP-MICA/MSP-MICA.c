/*
 ============================================================================
 Name        : MSP-MICA.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "msp.h"

#include <stddef.h>


/*
char* getBytesFromFile(FILE* entrada, size_t *tam_archivo);

int main(int argc, char *argv[]) {

	inicializarMSP();

	FILE *entrada;


		char * NOM_ARCHIVO = malloc(30);
		NOM_ARCHIVO = "systemCalls.bc";

			if ((entrada = fopen(NOM_ARCHIVO, "r")) == NULL){
				perror(NOM_ARCHIVO);
				return EXIT_FAILURE;
			}

			size_t cantBytes = 0;
			char *literal = getBytesFromFile(entrada, &cantBytes);


			puts("Hola aca te mando la instruccion");
			puts(literal);

			fclose(entrada);

			puts("eso es to do el archivo");
			crearSegmento(1234, 20);
			crearSegmento(1234, 100);

	escribirMemoria(1234,1048576,literal,100);

	listarMarcos();

	//conexionConKernelYCPU();


	puts("ksdjflkdsfFREE");

	char* pepe = malloc(20);
	memset(pepe, '\0', 20);
	memcpy(pepe, solicitarMemoria(1234, 1049096, 20), 20);
	printf("%s\n", pepe);

	//int numero;

	free(literal);
		return 0;

	listarMarcos();




}


char* getBytesFromFile(FILE* entrada, size_t *tam_archivo){
	fseek(entrada, 0L, SEEK_END);
	*tam_archivo = ftell(entrada);
	char * literal = (char*) calloc(1, *tam_archivo);
	fseek(entrada, 0L, 0L);

	fgets(literal, *tam_archivo, entrada);
	return literal;

}
*/



int main() {

	inicializarMSP();

	conexionConKernelYCPU();

	pthread_join(hilo_consola_1, NULL);


	pthread_exit(0);



	//crearSegmento(1234, 50);
	//escribirMemoria(1234, 0, "11111111112222222222", 20);
	/*
	listarMarcos();

	char* buffer = solicitarMemoria(1234, 5, 3);

	printf("buffer: %s\n", buffer);

	free(buffer);

	puts("hola");*/


	//puts("hola");


/*	//PRUEBA PARA AUMENTARPROGRAMCOUNTER
	uint32_t programCounterAnterior = 1048838;
	uint32_t nuevoProgramCounter;
	int bytesASumar = 4;
	int numeroSegmento, numeroPagina, offset, numeroSegmentoViejo, numeroPaginaViejo, offsetViejo;

	nuevoProgramCounter = aumentarProgramCounter(programCounterAnterior, bytesASumar);

	obtenerUbicacionLogica(programCounterAnterior, &numeroSegmentoViejo, &numeroPaginaViejo, &offsetViejo);

	printf("VIEJO PC: Numero segmento: %d    numero pagina: %d     offset: %d\n", numeroSegmentoViejo, numeroPaginaViejo, offsetViejo);

	obtenerUbicacionLogica(nuevoProgramCounter, &numeroSegmento, &numeroPagina, &offset);

	printf("NUEVO PC: Numero segmento: %d    numero pagina: %d     offset: %d\n", numeroSegmento, numeroPagina, offset);

	uint32_t direccion = generarDireccionLogica(1, 1, 12);

	printf("direccion: %zu\n", direccion);

	obtenerUbicacionLogica(direccion, &numeroSegmento, &numeroPagina, &offset);

	printf("OTRO PC: Numero segmento: %d    numero pagina: %d     offset: %d\n", numeroSegmento, numeroPagina, offset);*/




	return 0;


}
