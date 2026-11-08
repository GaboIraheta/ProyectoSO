#ifndef INPUT_H
#define INPUT_H

#include "common.h"

void input_init(SimulationInput *input);

int input_read_from_file(
    const char *path,
    SimulationInput *input
);

int input_read_interactive(SimulationInput *input);

int input_validate(const SimulationInput *input);

void input_print(const SimulationInput *input);

void input_free(SimulationInput *input);

#endif