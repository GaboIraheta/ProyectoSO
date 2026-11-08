#ifndef INPUT_H
#define INPUT_H

#include "common.h"

// vacia el input
void input_init(SimulationInput *input);

// Lee los datos desde un archivo
int input_read_from_file(const char *path, SimulationInput *input);

// Pide los datos al usuario por teclado
int input_read_interactive(SimulationInput *input);

// Verifica que los datos cumplan todos los límites
int input_validate(const SimulationInput *input);

// Muestra en pantalla los datos de entrada
void input_print(const SimulationInput *input);

// Libera la memoria de la secuencia de referencias
void input_free(SimulationInput *input);

#endif 
