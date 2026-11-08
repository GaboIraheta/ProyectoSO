#ifndef CLOCK_H
#define CLOCK_H

#include "../common/common.h"
#include <stddef.h> /* size_t */

/* Eventos que permiten REPRODUCIR el funcionamiento interno de Clock. */
typedef enum {
    CLOCK_EVENT_REFERENCE,      /* Comienza una nueva solicitud de pagina. */
    CLOCK_EVENT_HIT,            /* Pagina encontrada: su bit R pasa a 1. */
    CLOCK_EVENT_SECOND_CHANCE,  /* R=1 pasa a 0; avanza la manecilla. */
    CLOCK_EVENT_LOAD,           /* Carga en marco vacio; avanza la manecilla. */
    CLOCK_EVENT_REPLACE         /* Desaloja una pagina y carga otra. */
} ClockEventType;

typedef struct {
    ClockEventType type;
    int reference_index; /* 0, 1, 2... posicion de la referencia. */
    int page;            /* Pagina solicitada. */
    int frame_index;     /* Marco afectado; -1 para CLOCK_EVENT_REFERENCE. */
    int evicted_page;    /* Pagina desalojada o NO_EVICTION. */
    int r_before;        /* Bit R antes de la accion; -1 si no aplica. */
    int r_after;         /* Bit R despues de la accion; -1 si no aplica. */
    int hand_before;     /* Posicion de la manecilla antes de la accion. */
    int hand_after;      /* Posicion de la manecilla despues de la accion. */
} ClockTraceEvent;

typedef struct {
    ClockTraceEvent *events;
    size_t count;
    size_t capacity; /* Capacidad interna reservada. */
} ClockTrace;

/*
 * Ejecuta Clock como antes: solo genera el resultado compartido.
 * 'result' debe estar vacio / sin memoria pendiente de liberar.
 * En caso de exito, llamar a result_free(result).
 */
int clock_run(const SimulationInput *input, SimulationResult *result);

/*
 * Misma simulacion, pero tambien registra cada accion interna.
 * 'result' y 'trace' deben estar vacios (por ejemplo, inicializados con {0}).
 * En caso de exito, liberar con result_free(result) y clock_trace_free(trace).
 * En caso de error, la funcion libera sus reservas parciales.
 * NO se exige cambiar common/common.h ni el modulo LRU.
 */
int clock_run_traced(const SimulationInput *input, SimulationResult *result,
                     ClockTrace *trace);

/* Libera eventos y deja el seguimiento vacio. Admite NULL. */
void clock_trace_free(ClockTrace *trace);

#endif /* CLOCK_H */
