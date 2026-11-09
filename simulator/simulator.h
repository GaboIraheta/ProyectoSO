#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "../common/common.h"

/*
 * Ejecuta LRU y Clock con la MISMA entrada (mismas referencias y mismo
 * numero de marcos). Verifica que ambos resultados sean coherentes.
 *
 * En SIM_OK, lru y clock deben liberarse con result_free().
 * En SIM_ERROR ambos quedan liberados (seguros de pasar a result_free).
 * lru y clock deben venir inicializados en cero.
 */
int simulator_run(const SimulationInput *input,
                  SimulationResult *lru,
                  SimulationResult *clock);

/* Tabla paso a paso: referencia, marcos, acierto/fallo y pagina desalojada. */
void simulator_print_steps(const SimulationResult *result);

/* Totales de aciertos y fallos con sus porcentajes. */
void simulator_print_summary(const SimulationResult *result);

/* Compara dos resultados y declara cual tuvo menos fallos. */
void simulator_print_comparison(const SimulationResult *a,
                                const SimulationResult *b);

/* Porcentajes (0.0 si no hay pasos registrados). */
double simulator_hit_rate(const SimulationResult *result);
double simulator_fault_rate(const SimulationResult *result);

/* Atajo: imprime entrada, ambas tablas, resumenes y comparacion. */
void simulator_print_report(const SimulationInput *input,
                            const SimulationResult *lru,
                            const SimulationResult *clock);

#endif /* SIMULATOR_H */
