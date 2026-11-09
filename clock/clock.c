#include "clock.h"

typedef struct {
    int *frames;                  /* Pagina almacenada en cada marco. */
    unsigned char *reference_bits;/* Bit R de cada marco (0 o 1). */
    int num_frames;
    int hand;                     /* Siguiente marco que examina Clock. */
} ClockState;

/* Valida lo imprescindible sin depender del modulo de lectura de archivos. */
static int clock_validate_input(const SimulationInput *input)
{
    if (input == NULL ||
        input->num_frames < MIN_FRAMES || input->num_frames > MAX_FRAMES ||
        input->num_references <= 0 || input->num_references > MAX_REFERENCES ||
        input->references == NULL) {
        return SIM_ERROR;
    }

    for (int i = 0; i < input->num_references; i++) {
        if (input->references[i] < 0 ||
            input->references[i] > MAX_PAGE_NUMBER) {
            return SIM_ERROR;
        }
    }
    return SIM_OK;
}

static void clock_state_destroy(ClockState *state)
{
    if (state == NULL) {
        return;
    }
    frames_destroy(&state->frames);
    free(state->reference_bits);
    state->reference_bits = NULL;
    state->num_frames = 0;
    state->hand = 0;
}

static int clock_state_init(ClockState *state, int num_frames)
{
    memset(state, 0, sizeof *state);

    state->frames = frames_create(num_frames);
    if (state->frames == NULL) {
        return SIM_ERROR;
    }

    /* calloc deja todos los bits R inicialmente en 0. */
    state->reference_bits = calloc((size_t)num_frames,
                                   sizeof *state->reference_bits);
    if (state->reference_bits == NULL) {
        fprintf(stderr, "Error: no hay memoria para los bits R de Clock.\n");
        clock_state_destroy(state);
        return SIM_ERROR;
    }

    state->num_frames = num_frames;
    state->hand = 0;
    return SIM_OK;
}

/* Busca un acierto; los marcos vacios no pueden contener paginas validas. */
static int clock_find_page(const ClockState *state, int page)
{
    for (int i = 0; i < state->num_frames; i++) {
        if (state->frames[i] == page) {
            return i;
        }
    }
    return -1;
}

/*
 * Busca un marco libre o con R=0 desde la posicion de la manecilla.
 * Cada marco ocupado con R=1 recibe una segunda oportunidad: se pone
 * a 0 y la manecilla avanza. El recorrido circular esta garantizado.
 */
static int clock_select_frame(ClockState *state)
{
    while (state->frames[state->hand] != EMPTY_FRAME &&
           state->reference_bits[state->hand] != 0) {
        state->reference_bits[state->hand] = 0;
        state->hand = (state->hand + 1) % state->num_frames;
    }
    return state->hand;
}

/* Reserva una posicion por referencia para la manecilla y todos los bits R. */
static int clock_history_init(ClockHistory *history, const SimulationInput *input)
{
    memset(history, 0, sizeof *history);
    history->num_frames = input->num_frames;
    history->num_steps = input->num_references;

    history->hand_history = malloc((size_t)input->num_references *
                                  sizeof *history->hand_history);
    history->r_history = malloc((size_t)input->num_references *
                               (size_t)input->num_frames *
                               sizeof *history->r_history);

    if (history->hand_history == NULL || history->r_history == NULL) {
        clock_history_free(history);
        return SIM_ERROR;
    }
    return SIM_OK;
}

/* Copia el estado justo despues de procesar una referencia. */
static void clock_history_save(ClockHistory *history,
                               const ClockState *state, int step)
{
    history->hand_history[step] = state->hand;
    memcpy(&history->r_history[(size_t)step * (size_t)state->num_frames],
           state->reference_bits,
           (size_t)state->num_frames * sizeof *state->reference_bits);
}

void clock_history_free(ClockHistory *history)
{
    if (history == NULL) {
        return;
    }
    free(history->hand_history);
    free(history->r_history);
    memset(history, 0, sizeof *history);
}

/* La logica de reemplazo es la misma: el historial es opcional. */
static int clock_execute(const SimulationInput *input,
                         SimulationResult *result, ClockHistory *history)
{
    if (result == NULL || clock_validate_input(input) != SIM_OK) {
        fprintf(stderr, "Error: parametros de entrada invalidos para Clock.\n");
        return SIM_ERROR;
    }

    ClockState state;
    if (clock_state_init(&state, input->num_frames) != SIM_OK) {
        return SIM_ERROR;
    }

    if (result_init(result, "Clock", input) != SIM_OK) {
        clock_state_destroy(&state);
        return SIM_ERROR;
    }

    if (history != NULL && clock_history_init(history, input) != SIM_OK) {
        result_free(result);
        clock_state_destroy(&state);
        return SIM_ERROR;
    }

    for (int i = 0; i < input->num_references; i++) {
        const int page = input->references[i];
        const int found = clock_find_page(&state, page);
        const bool hit = (found != -1);
        int evicted_page = NO_EVICTION;

        if (hit) {
            /* Acierto: R=1 y la manecilla NO se mueve. */
            state.reference_bits[found] = 1;
        } else {
            /* Fallo: se ocupa un marco vacio o se aplica segunda oportunidad. */
            const int target = clock_select_frame(&state);
            evicted_page = state.frames[target];
            state.frames[target] = page;
            state.reference_bits[target] = 1;
            state.hand = (target + 1) % state.num_frames;
        }

        if (result_record_step(result, page, hit, evicted_page,
                               state.frames) != SIM_OK) {
            result_free(result);
            clock_state_destroy(&state);
            if (history != NULL) {
                clock_history_free(history);
            }
            return SIM_ERROR;
        }

        if (history != NULL) {
            clock_history_save(history, &state, i);
        }
    }

    clock_state_destroy(&state);
    return SIM_OK;
}

int clock_run(const SimulationInput *input, SimulationResult *result)
{
    return clock_execute(input, result, NULL);
}

int clock_run_with_history(const SimulationInput *input,
                           SimulationResult *result, ClockHistory *history)
{
    if (history == NULL) {
        return SIM_ERROR;
    }
    return clock_execute(input, result, history);
}
