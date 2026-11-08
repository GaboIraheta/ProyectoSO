#include "../clock/clock.h"
#include <stdio.h>
#include <string.h>

/* Reproduce la secuencia de eventos y compara cada fotografia de los marcos
   con el resultado compartido, ademas de verificar R y la manecilla. */
static int check_case(int num_frames, int *pages, int n,
                      int expected_hits, int expected_faults)
{
    SimulationInput input = { num_frames, pages, n };
    SimulationResult result = {0};
    ClockTrace trace = {0};
    SimulationResult plain = {0};
    int ok = 1;
    if (clock_run_traced(&input, &result, &trace) != SIM_OK) return 0;
    if (clock_run(&input, &plain) != SIM_OK) {
        result_free(&result); clock_trace_free(&trace); return 0;
    }
    if (result.hits != expected_hits || result.faults != expected_faults ||
        result.steps_recorded != n || plain.hits != result.hits ||
        plain.faults != result.faults || plain.steps_recorded != result.steps_recorded) ok = 0;

    int *frames = frames_create(num_frames);
    unsigned char *bits = calloc((size_t)num_frames, sizeof *bits);
    if (frames == NULL || bits == NULL) ok = 0;
    int hand = 0;
    int steps_seen = 0;
    int hits_seen = 0;
    int faults_seen = 0;
    if (frames != NULL && bits != NULL) {
        for (size_t i = 0; i < trace.count; ++i) {
            ClockTraceEvent *e = &trace.events[i];
            if (e->reference_index < 0 || e->reference_index >= n ||
                e->hand_before != hand || e->hand_after < 0 ||
                e->hand_after >= num_frames || e->page != pages[e->reference_index]) {
                ok = 0; break;
            }
            if (e->type == CLOCK_EVENT_REFERENCE) {
                if (e->frame_index != -1 || e->reference_index != steps_seen) ok = 0;
                steps_seen++;
            } else {
                if (e->frame_index < 0 || e->frame_index >= num_frames ||
                    e->r_before != bits[e->frame_index]) { ok = 0; break; }
                switch (e->type) {
                    case CLOCK_EVENT_HIT:
                        hits_seen++;
                        if (frames[e->frame_index] != e->page || e->r_after != 1 ||
                            e->hand_after != hand) ok = 0;
                        break;
                    case CLOCK_EVENT_SECOND_CHANCE:
                        if (frames[e->frame_index] == EMPTY_FRAME ||
                            e->r_before != 1 || e->r_after != 0 ||
                            e->hand_after != (hand + 1) % num_frames ||
                            e->frame_index != hand) ok = 0;
                        break;
                    case CLOCK_EVENT_LOAD:
                        faults_seen++;
                        if (frames[e->frame_index] != EMPTY_FRAME ||
                            e->evicted_page != NO_EVICTION ||
                            e->frame_index != hand || e->r_after != 1) ok = 0;
                        frames[e->frame_index] = e->page;
                        break;
                    case CLOCK_EVENT_REPLACE:
                        faults_seen++;
                        if (frames[e->frame_index] == EMPTY_FRAME ||
                            e->evicted_page != frames[e->frame_index] ||
                            e->r_before != 0 || e->r_after != 1 ||
                            e->frame_index != hand) ok = 0;
                        frames[e->frame_index] = e->page;
                        break;
                    default:
                        ok = 0;
                }
                bits[e->frame_index] = (unsigned char)e->r_after;
            }
            hand = e->hand_after;
            if (i + 1 == trace.count ||
                e->reference_index != trace.events[i+1].reference_index) {
                const int *expected = result_frames_at(&result, e->reference_index);
                if (memcmp(expected, frames, (size_t)num_frames * sizeof *frames) != 0)
                    ok = 0;
                if (memcmp(expected, result_frames_at(&plain, e->reference_index),
                           (size_t)num_frames * sizeof *frames) != 0) ok = 0;
                if (result.steps[e->reference_index].page != plain.steps[e->reference_index].page ||
                    result.steps[e->reference_index].hit != plain.steps[e->reference_index].hit ||
                    result.steps[e->reference_index].evicted_page != plain.steps[e->reference_index].evicted_page)
                    ok = 0;
            }
            if (!ok) break;
        }
        if (steps_seen != n || hits_seen != expected_hits || faults_seen != expected_faults)
            ok = 0;
    }
    frames_destroy(&frames);
    free(bits);
    clock_trace_free(&trace);
    result_free(&result);
    result_free(&plain);
    return ok;
}

int main(void)
{
    int a[] = {1,2,3,2,4};
    int classic[] = {7,0,1,2,0,3,0,4,2,3,0,3,2,1,2,0,1,7,0,1};
    int clock_wins[] = {2,1,4,3,6,2,3,5,3,2,6,3,1,5,1,2,6,3,5,1};
    int lru_wins[] = {2,5,3,2,3,4,2,3,5,3,1,2,3,1,3,4,3,1};
    int loop[] = {1,2,3,4,5,1,2,3,4,5,1,2,3,4,5};
    int phases[] = {
        1,2,3,1,2,3,4,1,2,3,4,1,6,7,8,9,6,7,8,9,
        6,7,1,2,3,4,1,2,3,4,10,11,12,10,11,12,1,2,3,4
    };
    int one[] = {1,1,2,2,1};
    int allhit[] = {0,0,0,0};
    struct { const char *name; int nframes; int *pages; int n; int hits; int faults; } cases[] = {
        {"Simple", 3, a, 5, 1, 4},
        {"Clasico", 3, classic, 20, 6, 14},
        {"LRU gana", 3, lru_wins, 18, 6, 12},
        {"Clock gana", 4, clock_wins, 20, 11, 9},
        {"Bucle", 4, loop, 15, 0, 15},
        {"Fases", 5, phases, 40, 21, 19},
        {"Un marco", 1, one, 5, 2, 3},
        {"Solo aciertos tras carga", 3, allhit, 4, 3, 1}
    };
    int ok = 1;
    for (size_t i = 0; i < sizeof cases/sizeof cases[0]; ++i) {
        int success = check_case(cases[i].nframes, cases[i].pages,
                                 cases[i].n, cases[i].hits, cases[i].faults);
        printf("%-26s : %s\n", cases[i].name, success ? "OK" : "FALLO");
        if (!success) ok = 0;
    }
    /* Prueba de error de entrada. */
    SimulationInput bad = {0};
    SimulationResult empty = {0};
    if (clock_run(&bad, &empty) != SIM_ERROR) ok = 0;
    if (empty.steps != NULL || empty.frame_history != NULL) ok = 0;
    /* Idempotencia de liberar estructura ya vacia. */
    ClockTrace trace = {0};
    clock_trace_free(&trace);
    clock_trace_free(&trace);
    puts(ok ? "TODAS LAS PRUEBAS SUPERADAS" : "HAY PRUEBAS FALLIDAS");
    return ok ? 0 : 1;
}
