/* Demostracion interactiva del algoritmo Clock.
   Compilar desde la raiz: gcc -std=c11 -Wall -Wextra -Werror -pedantic \
        tests/demo_clock.c clock/clock.c -o demo_clock
   Ejecutar: ./demo_clock
             ./demo_clock --auto
             ./demo_clock 3 1,2,3,2,4
             ./demo_clock --auto 4 2,1,4,3,6,2,3
*/
#include "../clock/clock.h"
#include <errno.h>
#include <limits.h>

static void show_state(const int *frames, const unsigned char *bits,
                       int count, int hand)
{
    printf("  Marco: ");
    for (int i = 0; i < count; ++i) printf("%5d", i);
    printf("\n  Pagina:");
    for (int i = 0; i < count; ++i) {
        if (frames[i] == EMPTY_FRAME) printf("%5s", "-");
        else printf("%5d", frames[i]);
    }
    printf("\n  Bit R: ");
    for (int i = 0; i < count; ++i) printf("%5u", (unsigned)bits[i]);
    printf("\n  Mano:  ");
    for (int i = 0; i < count; ++i) printf("%5s", i == hand ? "^" : " ");
    printf("   (proximo marco: %d)\n", hand);
}

static void apply_event(const ClockTraceEvent *event,
                        int *frames, unsigned char *bits, int *hand)
{
    if (event->type == CLOCK_EVENT_HIT ||
        event->type == CLOCK_EVENT_SECOND_CHANCE ||
        event->type == CLOCK_EVENT_LOAD ||
        event->type == CLOCK_EVENT_REPLACE) {
        bits[event->frame_index] = (unsigned char)event->r_after;
    }
    if (event->type == CLOCK_EVENT_LOAD || event->type == CLOCK_EVENT_REPLACE) {
        frames[event->frame_index] = event->page;
    }
    *hand = event->hand_after;
}

static void show_event(const ClockTraceEvent *event)
{
    switch (event->type) {
    case CLOCK_EVENT_REFERENCE:
        printf("REFERENCIA: se solicita la pagina %d.\n", event->page);
        break;
    case CLOCK_EVENT_HIT:
        printf("ACIERTO: pagina %d en marco %d, R: %d -> %d. La mano no avanza.\n",
               event->page, event->frame_index,
               event->r_before, event->r_after);
        break;
    case CLOCK_EVENT_SECOND_CHANCE:
        printf("SEGUNDA OPORTUNIDAD: marco %d tenia R=1; pasa a 0. Mano %d -> %d.\n",
               event->frame_index, event->hand_before, event->hand_after);
        break;
    case CLOCK_EVENT_LOAD:
        printf("FALLO: pagina %d cargada en marco vacio %d. R=1. Mano %d -> %d.\n",
               event->page, event->frame_index,
               event->hand_before, event->hand_after);
        break;
    case CLOCK_EVENT_REPLACE:
        printf("FALLO: pagina %d reemplaza a %d en marco %d. R=1. Mano %d -> %d.\n",
               event->page, event->evicted_page, event->frame_index,
               event->hand_before, event->hand_after);
        break;
    }
}

static int parse_sequence(const char *arg, int *pages, int max_count)
{
    int count = 0;
    const char *cursor = arg;
    while (*cursor != '\0') {
        char *end = NULL;
        errno = 0;
        long value = strtol(cursor, &end, 10);
        if (errno != 0 || end == cursor || value < 0 || value > MAX_PAGE_NUMBER ||
            count >= max_count) return -1;
        pages[count++] = (int)value;
        if (*end == '\0') break;
        if (*end != ',') return -1;
        cursor = end + 1;
        if (*cursor == '\0') return -1;
    }
    return count > 0 ? count : -1;
}

int main(int argc, char **argv)
{
    const int default_pages[] = {1, 2, 3, 2, 4};
    int pages[MAX_REFERENCES];
    int num_pages = 5;
    int num_frames = 3;
    int automatic = 0;
    int pos = 1;
    if (pos < argc && strcmp(argv[pos], "--auto") == 0) {
        automatic = 1;
        ++pos;
    }
    if (argc - pos == 2) {
        char *end = NULL;
        errno = 0;
        long n = strtol(argv[pos], &end, 10);
        if (errno != 0 || *end != '\0' || n < MIN_FRAMES || n > MAX_FRAMES) {
            fprintf(stderr, "Numero de marcos invalido.\n");
            return 1;
        }
        num_frames = (int)n;
        num_pages = parse_sequence(argv[pos + 1], pages, MAX_REFERENCES);
        if (num_pages < 0) {
            fprintf(stderr, "Secuencia invalida. Usar, por ejemplo: 1,2,3,2,4\n");
            return 1;
        }
    } else if (argc - pos == 0) {
        memcpy(pages, default_pages, sizeof default_pages);
    } else {
        fprintf(stderr, "Uso: %s [--auto] [num_marcos lista_separada_por_comas]\n", argv[0]);
        return 1;
    }

    SimulationInput input = {
        .num_frames = num_frames,
        .references = pages,
        .num_references = num_pages
    };
    SimulationResult result = {0};
    ClockTrace trace = {0};
    if (clock_run_traced(&input, &result, &trace) != SIM_OK) return 1;

    int *frames = frames_create(num_frames);
    unsigned char *bits = calloc((size_t)num_frames, sizeof *bits);
    if (frames == NULL || bits == NULL) {
        fprintf(stderr, "Error: no hay memoria para la visualizacion.\n");
        frames_destroy(&frames);
        free(bits);
        result_free(&result);
        clock_trace_free(&trace);
        return 1;
    }

    int hand = 0;
    printf("=== DEMOSTRACION CLOCK ===\nMarcos: %d | Referencias:", num_frames);
    for (int i = 0; i < num_pages; ++i) printf(" %d", pages[i]);
    printf("\n\n");
    for (size_t i = 0; i < trace.count; ++i) {
        const ClockTraceEvent *event = &trace.events[i];
        printf("Paso %d/%d | Evento %zu/%zu\n",
               event->reference_index + 1, num_pages, i + 1, trace.count);
        show_event(event);
        apply_event(event, frames, bits, &hand);
        show_state(frames, bits, num_frames, hand);
        if (i + 1 == trace.count ||
            trace.events[i + 1].reference_index != event->reference_index) {
            const StepRecord *step = &result.steps[event->reference_index];
            printf("  Resultado: %s", step->hit ? "ACIERTO" : "FALLO");
            if (step->evicted_page != NO_EVICTION)
                printf(" | Desalojada: %d", step->evicted_page);
            printf("\n");
        }
        printf("------------------------------------------------------------\n");
        if (!automatic) {
            char line[8];
            printf("Enter = siguiente evento (Ctrl+C = salir)... ");
            fflush(stdout);
            if (fgets(line, sizeof line, stdin) == NULL) automatic = 1;
            putchar('\n');
        }
    }
    printf("=== RESUMEN ===\nAciertos: %d | Fallos: %d | Total: %d\n",
           result.hits, result.faults, result.steps_recorded);
    printf("Aciertos: %.2f%% | Fallos: %.2f%%\n",
           100.0 * result.hits / result.steps_recorded,
           100.0 * result.faults / result.steps_recorded);

    frames_destroy(&frames);
    free(bits);
    result_free(&result);
    clock_trace_free(&trace);
    return 0;
}
