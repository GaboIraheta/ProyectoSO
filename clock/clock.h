#ifndef CLOCK_H
#define CLOCK_H

#include "../common/common.h"

/*
 * Ejecuta el algoritmo de reemplazo Clock (segunda oportunidad).
 *
 * Requisitos:
 *   - input contiene una cantidad de marcos y referencias validas.
 *   - result apunta a una estructura que no contiene recursos pendientes
 *     de liberar (si se reutiliza, llamar antes a result_free).
 *
 * En SIM_OK, result contiene un StepRecord y una fotografia de los marcos
 * por cada referencia. El llamador debe liberar result con result_free().
 * En SIM_ERROR, si llego a inicializarse, result se libera internamente.
 */
int clock_run(const SimulationInput *input, SimulationResult *result);

/* Una instantanea de hand y bits R por cada referencia procesada. */
typedef struct {
    int num_steps;
    int num_frames;
    int *hand_history;             /* Posicion de la manecilla por paso. */
    unsigned char *r_history;     /* Bits R: [paso * num_frames + marco]. */
} ClockHistory;

/* Se utiliza en lugar de clock_run cuando se necesita el historial. */
int clock_run_with_history(const SimulationInput *input,
                           SimulationResult *result,
                           ClockHistory *history);

/* Liberar el historial despues de mostrarlo. */
void clock_history_free(ClockHistory *history);

#endif /* CLOCK_H */
