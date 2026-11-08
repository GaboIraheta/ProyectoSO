#include "clock.h"
#include <stdint.h> /* SIZE_MAX */

typedef struct {
    int *frames;
    unsigned char *reference_bits;
    int num_frames;
    int hand;
} ClockState;

static int clock_validate_input(const SimulationInput *input)
{
    if (input == NULL ||
        input->num_frames < MIN_FRAMES || input->num_frames > MAX_FRAMES ||
        input->num_references <= 0 || input->num_references > MAX_REFERENCES ||
        input->references == NULL) {
        return SIM_ERROR;
    }
    for (int i = 0; i < input->num_references; i++) {
        if (input->references[i] < 0 || input->references[i] > MAX_PAGE_NUMBER) {
            return SIM_ERROR;
        }
    }
    return SIM_OK;
}

static void clock_state_destroy(ClockState *state)
{
    if (state == NULL) return;
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
    if (state->frames == NULL) return SIM_ERROR;

    state->reference_bits = calloc((size_t)num_frames, sizeof *state->reference_bits);
    if (state->reference_bits == NULL) {
        fprintf(stderr, "Error: no hay memoria para los bits R de Clock.\n");
        clock_state_destroy(state);
        return SIM_ERROR;
    }
    state->num_frames = num_frames;
    state->hand = 0;
    return SIM_OK;
}

static int clock_find_page(const ClockState *state, int page)
{
    for (int i = 0; i < state->num_frames; i++) {
        if (state->frames[i] == page) return i;
    }
    return -1;
}

void clock_trace_free(ClockTrace *trace)
{
    if (trace == NULL) return;
    free(trace->events);
    memset(trace, 0, sizeof *trace);
}

/* Guarda eventos compactos: el modulo visual puede reconstruir los arreglos
   a partir de eventos.*/
static int clock_trace_append(ClockTrace *trace, ClockTraceEvent event)
{
    if (trace == NULL) return SIM_OK; /* Modo compatible: sin seguimiento. */
    if (trace->count == trace->capacity) {
        size_t new_capacity = (trace->capacity == 0 ? 32 : trace->capacity * 2);
        if (new_capacity < trace->capacity ||
            new_capacity > SIZE_MAX / sizeof *trace->events) {
            return SIM_ERROR;
        }
        ClockTraceEvent *updated = realloc(
            trace->events, new_capacity * sizeof *trace->events);
        if (updated == NULL) return SIM_ERROR;
        trace->events = updated;
        trace->capacity = new_capacity;
    }
    trace->events[trace->count++] = event;
    return SIM_OK;
}

static int clock_record_event(ClockTrace *trace, ClockEventType type,
                              int step, int page, int frame, int victim,
                              int r_before, int r_after,
                              int hand_before, int hand_after)
{
    ClockTraceEvent event = {
        .type = type,
        .reference_index = step,
        .page = page,
        .frame_index = frame,
        .evicted_page = victim,
        .r_before = r_before,
        .r_after = r_after,
        .hand_before = hand_before,
        .hand_after = hand_after
    };
    return clock_trace_append(trace, event);
}

/* Busca un marco vacio o con R=0. Registra cada segunda oportunidad. */
static int clock_select_frame(ClockState *state, ClockTrace *trace,
                              int step, int page)
{
    while (state->frames[state->hand] != EMPTY_FRAME &&
           state->reference_bits[state->hand] != 0) {
        int frame = state->hand;
        int next = (frame + 1) % state->num_frames;
        if (clock_record_event(trace, CLOCK_EVENT_SECOND_CHANCE,
                               step, page, frame, NO_EVICTION,
                               1, 0, frame, next) != SIM_OK) {
            return -1;
        }
        state->reference_bits[frame] = 0;
        state->hand = next;
    }
    return state->hand;
}

int clock_run_traced(const SimulationInput *input, SimulationResult *result,
                     ClockTrace *trace)
{
    if (result == NULL || clock_validate_input(input) != SIM_OK) {
        fprintf(stderr, "Error: parametros de entrada invalidos para Clock.\n");
        return SIM_ERROR;
    }
    /* Si trace != NULL, debe venir vacio, igual que result. */
    ClockState state;
    if (clock_state_init(&state, input->num_frames) != SIM_OK) return SIM_ERROR;

    if (result_init(result, "Clock", input) != SIM_OK) {
        clock_state_destroy(&state);
        return SIM_ERROR;
    }

    for (int i = 0; i < input->num_references; i++) {
        const int page = input->references[i];
        const int found = clock_find_page(&state, page);
        const bool hit = found != -1;
        int evicted_page = NO_EVICTION;

        if (clock_record_event(trace, CLOCK_EVENT_REFERENCE,
                               i, page, -1, NO_EVICTION, -1, -1,
                               state.hand, state.hand) != SIM_OK) goto fail;

        if (hit) {
            int previous_r = state.reference_bits[found];
            if (clock_record_event(trace, CLOCK_EVENT_HIT,
                                   i, page, found, NO_EVICTION,
                                   previous_r, 1, state.hand, state.hand) != SIM_OK)
                goto fail;
            state.reference_bits[found] = 1; /* Acierto: mano no avanza. */
        } else {
            int target = clock_select_frame(&state, trace, i, page);
            if (target < 0) goto fail;
            evicted_page = state.frames[target];
            ClockEventType type = evicted_page == EMPTY_FRAME
                ? CLOCK_EVENT_LOAD : CLOCK_EVENT_REPLACE;
            int previous_r = state.reference_bits[target];
            int next = (target + 1) % state.num_frames;

            if (clock_record_event(trace, type, i, page, target, evicted_page,
                                   previous_r, 1, state.hand, next) != SIM_OK)
                goto fail;

            state.frames[target] = page;
            state.reference_bits[target] = 1;
            state.hand = next;
        }
        if (result_record_step(result, page, hit, evicted_page,
                               state.frames) != SIM_OK) goto fail;
    }

    clock_state_destroy(&state);
    return SIM_OK;

fail:
    fprintf(stderr, "Error: no fue posible registrar la simulacion Clock.\n");
    result_free(result);
    clock_trace_free(trace);
    clock_state_destroy(&state);
    return SIM_ERROR;
}

int clock_run(const SimulationInput *input, SimulationResult *result)
{
    return clock_run_traced(input, result, NULL);
}
