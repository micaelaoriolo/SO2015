/*
 * consola-msp.c
 *
 *  Created on: 30/10/2014
 *      Author: utnso
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <commons/string.h>
#include <commons/log.h>
#include "msp.h"

#define EXIT_ATTEMPT 1
#define TAM_ENTRADA 60

int consola_msp() {

	printf("MSP - CONSOLA\n");
	printf("Para ver la lista de comandos, escriba 'ayuda'.\n");


	while(true)
	{
		printf("\n> ");

		char* buffer = malloc(256);

		scanf("%[^\n]", buffer);
		while(getchar()!='\n');

		if (buscarComando(buffer) == EXIT_FAILURE)
		{
			puts("Error, comando inválido.");
		}

		free(buffer);

	}

	return EXIT_SUCCESS;
}

int buscarComando(char* buffer)
{
	char** substrings = string_split((char*)buffer, " ");

	//Armo los comandos

	if ((string_equals_ignore_case(substrings[0], "ayuda")) && (substrings[1] == NULL))
	{
		printf("AYUDA\n");
		printf("Los comandos disponibles son:\n");
		printf("\tcrear segmento [PID] [tamaño]\n");
		printf("\tdestruir segmento [PID] [dirección base]\n");
		printf("\tescribir memoria [PID] [dirección base] [bytes a escribir] [tamaño]\n");
		printf("\tleer memoria [PID] [dirección base] [tamaño]\n");
		printf("\ttabla de segmentos\n");
		printf("\ttabla de paginas [PID]\n");
		printf("\tlistar marcos\n");

		return EXIT_SUCCESS;
	}

	if (string_equals_ignore_case(substrings[0], "tabla"))
	{
		if ((substrings[1] == NULL) || (!string_equals_ignore_case(substrings[1], "de")) || (substrings[2] == NULL) ||
				(!string_equals_ignore_case(substrings[2], "paginas")) || (substrings[3] == NULL) || (substrings[4] != NULL ))
		{
			if((substrings[1] == NULL) || (!string_equals_ignore_case(substrings[1], "de")) || (substrings[2] == NULL) ||
					(!string_equals_ignore_case(substrings[2], "segmentos")) || (substrings[3] != NULL))
			{
				liberarSubstrings(substrings);

				return EXIT_FAILURE;
			}
			else
			{
				//printf("La instruccion es %s %s %s\n", substrings[0], substrings[1], substrings[2]);

				consola = 1;
				tablaSegmentos();
				consola = 0;
				liberarSubstrings(substrings);

				return EXIT_SUCCESS;
			}
			liberarSubstrings(substrings);

			return EXIT_FAILURE;
		}

		consola = 1;
		tablaPaginas(atoi(substrings[3]));
		consola = 0;
		//printf("El comando es %s %s %s %d\n", substrings[0], substrings[1], substrings[2], atoi(substrings[3]));

		liberarSubstrings(substrings);


		return EXIT_SUCCESS;
	}


	if ((string_equals_ignore_case(substrings[0], "crear")))
	{
		if((substrings[1] == NULL) || (!string_equals_ignore_case(substrings[1], "segmento")) || (substrings[2] == NULL) || (substrings[3] == NULL) || (substrings[4] != NULL))
		{
			liberarSubstrings(substrings);

			return EXIT_FAILURE;
		}

		int pid = atoi(substrings[2]);
		int tamanio = atoi(substrings[3]);

		consola = 1;
		crearSegmento(pid, tamanio);
		consola = 0;

		//printf("La instruccion %s %s %d %d\n", substrings[0], substrings[1], pid, tamanio);

		liberarSubstrings(substrings);

		return EXIT_SUCCESS;
	}

	if ((string_equals_ignore_case(substrings[0], "destruir")))
	{
		if((substrings[1] == NULL) || (!string_equals_ignore_case(substrings[1], "segmento")) || (substrings[2] == NULL) || (substrings[3] == NULL) || (substrings[4] != NULL))
		{
			liberarSubstrings(substrings);

			return EXIT_FAILURE;
		}

		int pid = atoi(substrings[2]);
		int base = atoi(substrings[3]);

		consola = 1;
		destruirSegmento(pid, base);
		consola = 0;

		//printf("La instruccion %s %s %d %d\n", substrings[0], substrings[1], pid, base);

		liberarSubstrings(substrings);

		return EXIT_SUCCESS;
	}

	if ((string_equals_ignore_case(substrings[0], "escribir")))
	{
		if((substrings[1] == NULL) || (!string_equals_ignore_case(substrings[1], "memoria")) || (substrings[2] == NULL) || (substrings[3] == NULL) || (substrings[4] == NULL) || (substrings[5] == NULL) || (substrings[6] != NULL))
		{
			liberarSubstrings(substrings);

			return EXIT_FAILURE;
		}

		int pid = atoi(substrings[2]);
		uint32_t direccion = atoi(substrings[3]);
		int tamanio = atoi(substrings[5]);

		consola = 1;
		escribirMemoria(pid, direccion, substrings[4], tamanio);
		consola = 0;

		//printf("La instruccion %s %s %d %d %s %d\n", substrings[0], substrings[1], pid, direccion, substrings[4], tamanio);


		liberarSubstrings(substrings);


		return EXIT_SUCCESS;
	}


	if ((string_equals_ignore_case(substrings[0], "leer")))
	{
		if((substrings[1] == NULL) || (!string_equals_ignore_case(substrings[1], "memoria")) || (substrings[2] == NULL) || (substrings[3] == NULL) || (substrings[4] == NULL) || (substrings [5] != NULL))
		{
			liberarSubstrings(substrings);

			return EXIT_FAILURE;
		}



		int pid = atoi(substrings[2]);
		uint32_t direccion = atoi(substrings[3]);
		int tamanio = atoi(substrings[4]);


		void* recibido;

		consola = 1;
		recibido = solicitarMemoria(pid, direccion, tamanio);

		consola = 0;

		if (recibido != NULL || tamanio == 0)
		{
			printf("Lo que se obtuvo de la memoria es: %s", (char*)recibido);
			//puts("Lo que se obtuvo de la memoria es:");
			//puts((char*)recibido);
		}

		free(recibido);

		//printf("La instruccion %s %s %d %d %d\n", substrings[0], substrings[1], pid, direccion, tamanio);

		liberarSubstrings(substrings);


		return EXIT_SUCCESS;
	}

	if ((string_equals_ignore_case(substrings[0], "listar")))
	{
		if((substrings[1] == NULL) || (!string_equals_ignore_case(substrings[1], "marcos")) || (substrings[2] != NULL))
		{
			liberarSubstrings(substrings);

			return EXIT_FAILURE;
		}

		consola = 1;
		listarMarcos();
		consola = 0;

		//printf("La instruccion %s %s\n", substrings[0], substrings[1]);

		liberarSubstrings(substrings);

		return EXIT_SUCCESS;
	}

	if ((string_equals_ignore_case(substrings[0], "salir")))
	{
		if (substrings[1] != NULL)
		{
			liberarSubstrings(substrings);

			return EXIT_FAILURE;
		}

		free(buffer);
		liberarSubstrings(substrings);


		consola = 1;
		terminarMSP();
		consola = 0;


		exit(0);
	}

	liberarSubstrings(substrings);
	return EXIT_FAILURE;

}

void liberarSubstrings(char** sub)
{
	int i = 0;
	while (sub[i] != NULL)
	{
		free(sub[i]);
		i++;
	}

	free(sub);

}






