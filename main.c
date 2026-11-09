#include "clock/clock.h"
#include "common/compat.h"
#include "common/input.h"
#include "lru/lru.h"
#include "simulator/simulator.h"

typedef enum {
    OPTION_EXIT = 0,
    OPTION_LRU = 1,
    OPTION_CLOCK = 2,
    OPTION_BOTH = 3
} MenuOption;

/* Devuelve la opcion elegida; OPTION_EXIT tambien si termina la entrada. */
static MenuOption menu_choose(void)
{
    char *line = NULL;
    size_t size = 0;
    MenuOption choice = OPTION_EXIT;

    while (true) {
        printf("\n=== Menu ===\n"
               "1) LRU\n"
               "2) Clock\n"
               "3) Ambos (con comparacion)\n"
               "0) Salir\n"
               "Opcion: ");
        fflush(stdout);

        if (compat_getline(&line, &size, stdin) == -1) {
            printf("\n");
            break;
        }

        char *end = NULL;
        long value = strtol(line, &end, 10);

        while (end != line && (*end == ' ' || *end == '\t' || *end == '\r')) {
            ++end;
        }

        if (end != line && *end == '\n' && value >= OPTION_EXIT &&
            value <= OPTION_BOTH) {
            choice = (MenuOption)value;
            break;
        }

        fprintf(stderr, "Opcion invalida: elija un numero entre 0 y 3.\n");
    }

    free(line);
    return choice;
}

static int run_option(const SimulationInput *input, MenuOption option)
{
    SimulationResult lru = {0};
    SimulationResult clock = {0};

    input_print(input);

    switch (option) {
    case OPTION_LRU:
        if (lru_run(input, &lru) != SIM_OK) {
            return SIM_ERROR;
        }
        simulator_print_steps(&lru);
        simulator_print_summary(&lru);
        result_free(&lru);
        return SIM_OK;

    case OPTION_CLOCK:
        if (clock_run(input, &clock) != SIM_OK) {
            return SIM_ERROR;
        }
        simulator_print_steps(&clock);
        simulator_print_summary(&clock);
        result_free(&clock);
        return SIM_OK;

    case OPTION_BOTH:
        if (simulator_run(input, &lru, &clock) != SIM_OK) {
            return SIM_ERROR;
        }
        simulator_print_steps(&lru);
        simulator_print_summary(&lru);
        simulator_print_steps(&clock);
        simulator_print_summary(&clock);
        simulator_print_comparison(&lru, &clock);
        result_free(&lru);
        result_free(&clock);
        return SIM_OK;

    default:
        return SIM_ERROR;
    }
}

/*
 * Uso:
 *   ./build/simulador                 -> pide los datos por teclado
 *   ./build/simulador archivo.txt     -> lee los datos del archivo
 * Luego muestra un menu para elegir LRU, Clock o ambos sobre esos datos.
 */
int main(int argc, char **argv)
{
    if (argc > 2) {
        fprintf(stderr, "Uso: %s [archivo_de_entrada]\n", argv[0]);
        return EXIT_FAILURE;
    }

    SimulationInput input;
    input_init(&input);

    int status = (argc == 2)
        ? input_read_from_file(argv[1], &input)
        : input_read_interactive(&input);

    if (status != SIM_OK) {
        return EXIT_FAILURE;
    }

    int exit_code = EXIT_SUCCESS;
    MenuOption option;

    while ((option = menu_choose()) != OPTION_EXIT) {
        if (run_option(&input, option) != SIM_OK) {
            exit_code = EXIT_FAILURE;
            break;
        }
    }

    input_free(&input);
    return exit_code;
}
