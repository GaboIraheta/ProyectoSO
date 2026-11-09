/*
 * Pruebas del algoritmo Clock (segunda oportunidad).
 * Ejecutar desde la raiz del repositorio:
 * gcc -std=c11 -Wall -Wextra -Werror -pedantic tests/test_clock.c clock/clock.c -o test_clock.exe
 * .\test_clock.exe
 *
 * Usa ClockHistory, la version sencilla que conserva una instantanea
 * de los bits R y de la manecilla despues de cada referencia.
 */
#include "../clock/clock.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_basic_sequence(void)
{
    int references[] = {1, 2, 3, 2, 4};
    SimulationInput input = {.num_frames = 3, .references = references,
                             .num_references = 5};
    SimulationResult result = {0};

    assert(clock_run(&input, &result) == SIM_OK);
    assert(result.num_frames == 3);
    assert(result.num_steps == 5);
    assert(result.steps_recorded == 5);
    assert(result.hits == 1 && result.faults == 4);
    assert(result.steps[3].hit == true);
    assert(result.steps[4].page == 4);
    assert(result.steps[4].evicted_page == 1);

    const int *last = result_frames_at(&result, 4);
    assert(last != NULL);
    assert(last[0] == 4 && last[1] == 2 && last[2] == 3);
    result_free(&result);
}

static void test_repeated_references(void)
{
    int references[] = {5, 5, 5, 5, 5};
    SimulationInput input = {.num_frames = 3, .references = references,
                             .num_references = 5};
    SimulationResult result = {0};

    assert(clock_run(&input, &result) == SIM_OK);
    assert(result.faults == 1 && result.hits == 4);
    for (int i = 0; i < result.num_steps; ++i) {
        assert(result.steps[i].evicted_page == NO_EVICTION);
    }
    result_free(&result);
}

static void test_single_frame(void)
{
    int references[] = {1, 2, 3, 2};
    const int evicted[] = {NO_EVICTION, 1, 2, 3};
    SimulationInput input = {.num_frames = 1, .references = references,
                             .num_references = 4};
    SimulationResult result = {0};
    ClockHistory history = {0};

    assert(clock_run_with_history(&input, &result, &history) == SIM_OK);
    assert(result.faults == 4 && result.hits == 0);
    for (int i = 0; i < 4; ++i) {
        assert(result.steps[i].evicted_page == evicted[i]);
        assert(history.hand_history[i] == 0);
        assert(history.r_history[i] == 1);
    }
    assert(result_frames_at(&result, 3)[0] == 2);
    clock_history_free(&history);
    result_free(&result);
}

static void test_page_zero(void)
{
    int references[] = {0, 1, 0, 2};
    SimulationInput input = {.num_frames = 2, .references = references,
                             .num_references = 4};
    SimulationResult result = {0};

    assert(clock_run(&input, &result) == SIM_OK);
    assert(result.faults == 3 && result.hits == 1);
    assert(result.steps[2].hit == true);
    /* En Clock, despues de dos segundas oportunidades, sale la pagina 0. */
    assert(result.steps[3].evicted_page == 0);
    result_free(&result);
}

static void test_empty_frames(void)
{
    int references[] = {8};
    SimulationInput input = {.num_frames = 3, .references = references,
                             .num_references = 1};
    SimulationResult result = {0};

    assert(clock_run(&input, &result) == SIM_OK);
    const int *frames = result_frames_at(&result, 0);
    assert(frames != NULL);
    assert(frames[0] == 8);
    assert(frames[1] == EMPTY_FRAME && frames[2] == EMPTY_FRAME);
    assert(result.steps[0].evicted_page == NO_EVICTION);
    assert(result_frames_at(&result, -1) == NULL);
    assert(result_frames_at(&result, 1) == NULL);
    result_free(&result);
}

static void test_invalid_input(void)
{
    int references[] = {1, 2, 3};
    SimulationInput input = {.num_frames = 0, .references = references,
                             .num_references = 3};
    SimulationResult result = {0};
    ClockHistory history = {0};

    assert(clock_run(NULL, &result) == SIM_ERROR);
    assert(clock_run(&input, &result) == SIM_ERROR);
    input.num_frames = 3;
    input.references = NULL;
    assert(clock_run(&input, &result) == SIM_ERROR);
    input.references = references;
    input.num_references = 0;
    assert(clock_run(&input, &result) == SIM_ERROR);
    input.num_references = 3;
    references[1] = -5;
    assert(clock_run(&input, &result) == SIM_ERROR);
    references[1] = MAX_PAGE_NUMBER + 1;
    assert(clock_run(&input, &result) == SIM_ERROR);
    references[1] = 2;
    input.num_frames = MAX_FRAMES + 1;
    assert(clock_run(&input, &result) == SIM_ERROR);
    input.num_frames = 3;
    assert(clock_run_with_history(&input, &result, NULL) == SIM_ERROR);
    assert(result.steps == NULL && result.frame_history == NULL);
    assert(history.hand_history == NULL && history.r_history == NULL);
}

static void test_result_consistency(void)
{
    int references[] = {2, 1, 2, 3, 1, 4, 2};
    SimulationInput input = {.num_frames = 3, .references = references,
                             .num_references = 7};
    SimulationResult result = {0};

    assert(clock_run(&input, &result) == SIM_OK);
    assert(result.steps_recorded == input.num_references);
    assert(result.hits + result.faults == input.num_references);
    for (int i = 0; i < input.num_references; ++i) {
        assert(result.steps[i].page == references[i]);
        const int *frames = result_frames_at(&result, i);
        assert(frames != NULL);
        int occurrences = 0;
        for (int j = 0; j < input.num_frames; ++j) {
            if (frames[j] == references[i]) ++occurrences;
        }
        assert(occurrences == 1);
    }
    result_free(&result);
}

static void test_clock_history(void)
{
    int references[] = {1, 2, 3, 2, 4};
    const int expected_hands[] = {1, 2, 0, 0, 1};
    const unsigned char expected_bits[] = {
        1, 0, 0,
        1, 1, 0,
        1, 1, 1,
        1, 1, 1,
        1, 0, 0
    };
    SimulationInput input = {.num_frames = 3, .references = references,
                             .num_references = 5};
    SimulationResult result = {0};
    SimulationResult without_history = {0};
    ClockHistory history = {0};

    assert(clock_run_with_history(&input, &result, &history) == SIM_OK);
    assert(history.num_steps == 5 && history.num_frames == 3);
    assert(history.hand_history != NULL && history.r_history != NULL);
    for (int i = 0; i < 5; ++i) {
        assert(history.hand_history[i] == expected_hands[i]);
    }
    for (int i = 0; i < 15; ++i) {
        assert(history.r_history[i] == expected_bits[i]);
    }

    /* Registrar el historial no debe cambiar la logica del algoritmo. */
    assert(clock_run(&input, &without_history) == SIM_OK);
    assert(result.hits == without_history.hits);
    assert(result.faults == without_history.faults);
    for (int i = 0; i < input.num_references; ++i) {
        assert(result.steps[i].page == without_history.steps[i].page);
        assert(result.steps[i].hit == without_history.steps[i].hit);
        assert(result.steps[i].evicted_page ==
               without_history.steps[i].evicted_page);
        const int *a = result_frames_at(&result, i);
        const int *b = result_frames_at(&without_history, i);
        assert(a != NULL && b != NULL);
        for (int j = 0; j < input.num_frames; ++j) {
            assert(a[j] == b[j]);
        }
    }
    clock_history_free(&history);
    result_free(&result);
    result_free(&without_history);
}

static void check_scenario(const char *name, int num_frames,
                           int *references, int count,
                           int expected_faults)
{
    SimulationInput input = {.num_frames = num_frames,
                             .references = references,
                             .num_references = count};
    SimulationResult result = {0};
    ClockHistory history = {0};

    assert(clock_run_with_history(&input, &result, &history) == SIM_OK);
    assert(result.faults == expected_faults);
    assert(result.hits == count - expected_faults);
    assert(result.steps_recorded == count);
    assert(history.num_steps == count);
    assert(history.num_frames == num_frames);
    printf("  OK: %s\n", name);
    clock_history_free(&history);
    result_free(&result);
}

static void test_project_scenarios(void)
{
    int a[] = {7,0,1,2,0,3,0,4,2,3,0,3,2,1,2,0,1,7,0,1};
    int b[] = {2,5,3,2,3,4,2,3,5,3,1,2,3,1,3,4,3,1};
    int c[] = {2,1,4,3,6,2,3,5,3,2,6,3,1,5,1,2,6,3,5,1};
    int d[] = {1,2,3,4,5,1,2,3,4,5,1,2,3,4,5};
    int e[] = {1,2,3,1,2,3,4,1,2,3,4,1,6,7,8,9,6,7,8,9,
               6,7,1,2,3,4,1,2,3,4,10,11,12,10,11,12,1,2,3,4};

    check_scenario("Ejemplo 1 (clasico)", 3, a, (int)(sizeof a/sizeof a[0]), 14);
    check_scenario("Ejemplo 2 (LRU gana)", 3, b, (int)(sizeof b/sizeof b[0]), 12);
    check_scenario("Ejemplo 3 (Clock gana)", 4, c, (int)(sizeof c/sizeof c[0]), 9);
    check_scenario("Ejemplo 4 (bucle)", 4, d, (int)(sizeof d/sizeof d[0]), 15);
    check_scenario("Ejemplo 5 (fases)", 5, e, (int)(sizeof e/sizeof e[0]), 19);
}

int main(void)
{
    test_basic_sequence();
    test_repeated_references();
    test_single_frame();
    test_page_zero();
    test_empty_frames();
    test_invalid_input();
    test_result_consistency();
    test_clock_history();
    test_project_scenarios();
    puts("Todas las pruebas de Clock finalizaron correctamente.");
    return 0;
}
