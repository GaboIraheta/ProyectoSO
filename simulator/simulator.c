#define _POSIX_C_SOURCE 200809L

#include "simulator.h"

#include <unistd.h>

#include "../clock/clock.h"
#include "../lru/lru.h"

static int simulator_check_result(const SimulationInput *input,
                                  const SimulationResult *result)
{
    if (result->num_frames != input->num_frames ||
        result->steps_recorded != input->num_references ||
        result->hits + result->faults != input->num_references) {

        fprintf(stderr, "Error: resultado inconsistente de %s.\n",
                result->algorithm_name ? result->algorithm_name : "?");
        return SIM_ERROR;
    }

    for (int i = 0; i < input->num_references; ++i) {
        if (result->steps[i].page != input->references[i]) {
            fprintf(stderr,
                    "Error: %s no proceso la referencia #%d en orden.\n",
                    result->algorithm_name, i + 1);
            return SIM_ERROR;
        }
    }

    return SIM_OK;
}

int simulator_run(const SimulationInput *input,
                  SimulationResult *lru,
                  SimulationResult *clock)
{
    if (input == NULL || lru == NULL || clock == NULL) {
        fprintf(stderr, "Error: argumentos nulos en el simulador.\n");
        return SIM_ERROR;
    }

    if (lru_run(input, lru) != SIM_OK) {
        return SIM_ERROR;
    }

    if (clock_run(input, clock) != SIM_OK) {
        result_free(lru);
        return SIM_ERROR;
    }

    if (simulator_check_result(input, lru) != SIM_OK ||
        simulator_check_result(input, clock) != SIM_OK) {
        result_free(lru);
        result_free(clock);
        return SIM_ERROR;
    }

    return SIM_OK;
}

double simulator_hit_rate(const SimulationResult *result)
{
    if (result == NULL || result->steps_recorded <= 0) {
        return 0.0;
    }
    return 100.0 * result->hits / result->steps_recorded;
}

double simulator_fault_rate(const SimulationResult *result)
{
    if (result == NULL || result->steps_recorded <= 0) {
        return 0.0;
    }
    return 100.0 * result->faults / result->steps_recorded;
}

#define COLOR_RED   "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_RESET "\033[0m"

/* Solo usa color si la salida es una terminal y NO_COLOR no esta definido. */
static bool use_color(void)
{
    return isatty(STDOUT_FILENO) && getenv("NO_COLOR") == NULL;
}

/* Imprime texto coloreado ocupando "width" columnas visibles. */
static void print_colored(const char *text, const char *color, int width)
{
    const int pad = width - (int)strlen(text);

    if (use_color()) {
        printf("%s%s%s", color, text, COLOR_RESET);
    } else {
        printf("%s", text);
    }
    printf("%*s", pad > 0 ? pad : 0, "");
}

static void print_line(char c, int n)
{
    for (int i = 0; i < n; ++i) {
        putchar(c);
    }
    putchar('\n');
}

void simulator_print_steps(const SimulationResult *result)
{
    if (result == NULL || result->steps == NULL) {
        return;
    }

    /* Cada marco ocupa 5 columnas ("  7  " o "  -  "). */
    const int frames_width = result->num_frames * 5 + 2;
    const int total_width = 6 + 6 + frames_width + 10 + 11;

    printf("\n=== %s: simulacion paso a paso (%d marcos) ===\n",
           result->algorithm_name, result->num_frames);

    print_line('-', total_width);
    printf("%-6s%-6s%-*s%-10s%-11s\n",
           "Paso", "Ref", frames_width, "Marcos", "Resultado", "Desalojada");
    print_line('-', total_width);

    for (int i = 0; i < result->steps_recorded; ++i) {
        const StepRecord *step = &result->steps[i];
        const int *frames = result_frames_at(result, i);

        printf("%-6d%-6d[", i + 1, step->page);

        for (int f = 0; f < result->num_frames; ++f) {
            if (frames[f] == EMPTY_FRAME) {
                printf("%4s ", "-");
            } else {
                printf("%4d ", frames[f]);
            }
        }
        printf("] ");

        if (step->hit) {
            print_colored("ACIERTO", COLOR_GREEN, 10);
        } else {
            print_colored("FALLO", COLOR_RED, 10);
        }

        if (step->evicted_page == NO_EVICTION) {
            printf("%-11s\n", "-");
        } else {
            printf("%-11d\n", step->evicted_page);
        }
    }

    print_line('-', total_width);
}

void simulator_print_summary(const SimulationResult *result)
{
    if (result == NULL) {
        return;
    }

    printf("\n--- Resumen %s ---\n", result->algorithm_name);
    printf("Referencias : %d\n", result->steps_recorded);
    printf("Aciertos    : %d (%.1f%%)\n",
           result->hits, simulator_hit_rate(result));
    printf("Fallos      : %d (%.1f%%)\n",
           result->faults, simulator_fault_rate(result));
}

void simulator_print_comparison(const SimulationResult *a,
                                const SimulationResult *b)
{
    if (a == NULL || b == NULL) {
        return;
    }

    printf("\n=== Comparacion (%d marcos, %d referencias) ===\n",
           a->num_frames, a->steps_recorded);
    printf("%-12s%10s%12s%10s%12s\n",
           "Algoritmo", "Fallos", "% Fallos", "Aciertos", "% Aciertos");

    const SimulationResult *rows[2] = {a, b};
    for (int i = 0; i < 2; ++i) {
        printf("%-12s%10d%11.1f%%%10d%11.1f%%\n",
               rows[i]->algorithm_name,
               rows[i]->faults, simulator_fault_rate(rows[i]),
               rows[i]->hits, simulator_hit_rate(rows[i]));
    }

    if (a->faults == b->faults) {
        printf("\nResultado: empate, ambos con %d fallos.\n", a->faults);
        return;
    }

    const SimulationResult *best = a->faults < b->faults ? a : b;
    const SimulationResult *worst = best == a ? b : a;
    printf("\nResultado: %s tuvo menos fallos (%d contra %d, "
           "%d menos; diferencia de %.1f puntos porcentuales).\n",
           best->algorithm_name, best->faults, worst->faults,
           worst->faults - best->faults,
           simulator_fault_rate(worst) - simulator_fault_rate(best));
}

void simulator_print_report(const SimulationInput *input,
                            const SimulationResult *lru,
                            const SimulationResult *clock)
{
    if (input != NULL) {
        printf("\n=== Datos de entrada ===\n");
        printf("Marcos fisicos       : %d\n", input->num_frames);
        printf("Total de referencias : %d\n", input->num_references);
        printf("Secuencia            : ");
        for (int i = 0; i < input->num_references; ++i) {
            printf("%d%s", input->references[i],
                   i + 1 < input->num_references ? " " : "\n");
        }
    }

    simulator_print_steps(lru);
    simulator_print_summary(lru);
    simulator_print_steps(clock);
    simulator_print_summary(clock);
    simulator_print_comparison(lru, clock);
}
