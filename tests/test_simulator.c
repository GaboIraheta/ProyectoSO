#include "../common/input.h"
#include "../simulator/simulator.h"

#include <assert.h>
#include <math.h>

/* Ejecuta ambos algoritmos y verifica los totales esperados. */
static void check_scenario(const char *name, int frames, const int *refs,
                           int n, int lru_faults, int clock_faults)
{
    SimulationInput input = {
        .num_frames = frames,
        .references = (int *)refs,
        .num_references = n
    };
    SimulationResult lru = {0}, clock = {0};

    assert(simulator_run(&input, &lru, &clock) == SIM_OK);

    /* Misma cantidad de marcos y misma secuencia en ambos. */
    assert(lru.num_frames == frames && clock.num_frames == frames);
    assert(lru.num_steps == n && clock.num_steps == n);
    for (int i = 0; i < n; ++i) {
        assert(lru.steps[i].page == refs[i]);
        assert(clock.steps[i].page == refs[i]);
    }

    assert(lru.faults == lru_faults);
    assert(clock.faults == clock_faults);
    assert(lru.hits + lru.faults == n);
    assert(clock.hits + clock.faults == n);

    assert(fabs(simulator_fault_rate(&lru) + simulator_hit_rate(&lru) - 100.0) < 1e-9);
    assert(fabs(simulator_fault_rate(&clock) + simulator_hit_rate(&clock) - 100.0) < 1e-9);

    /* Un desalojo ocurre solo en fallos y nunca antes de llenar los marcos. */
    const SimulationResult *all[2] = {&lru, &clock};
    for (int a = 0; a < 2; ++a) {
        int evictions = 0;
        for (int i = 0; i < n; ++i) {
            if (all[a]->steps[i].hit) {
                assert(all[a]->steps[i].evicted_page == NO_EVICTION);
            } else if (all[a]->steps[i].evicted_page != NO_EVICTION) {
                ++evictions;
            }
        }
        assert(evictions <= all[a]->faults);
        assert(evictions >= all[a]->faults - frames);
    }

    printf("OK: %s (LRU=%d fallos, Clock=%d fallos)\n",
           name, lru.faults, clock.faults);

    result_free(&lru);
    result_free(&clock);
}

static void test_scenarios(void)
{
    int a[] = {7,0,1,2,0,3,0,4,2,3,0,3,2,1,2,0,1,7,0,1};
    int d[] = {1,2,3,4,5,1,2,3,4,5,1,2,3,4,5};

    check_scenario("clasico", 3, a, 20, 12, 14);
    check_scenario("bucle", 4, d, 15, 15, 15);
}

static void test_files(void)
{
    /* Los archivos de datos_prueba deben cargar y simularse sin error. */
    const char *files[] = {
        "datos_prueba/ejemplo1_clasico.txt",
        "datos_prueba/ejemplo2_lru_gana.txt",
        "datos_prueba/ejemplo3_clock_gana.txt",
        "datos_prueba/ejemplo4_bucle.txt",
        "datos_prueba/ejemplo5_fases.txt",
    };

    for (size_t i = 0; i < sizeof files / sizeof files[0]; ++i) {
        SimulationInput input;
        input_init(&input);
        assert(input_read_from_file(files[i], &input) == SIM_OK);

        SimulationResult lru = {0}, clock = {0};
        assert(simulator_run(&input, &lru, &clock) == SIM_OK);
        assert(lru.num_frames == clock.num_frames);
        assert(lru.steps_recorded == clock.steps_recorded);

        result_free(&lru);
        result_free(&clock);
        input_free(&input);
    }
    puts("OK: archivos de datos_prueba");
}

static void test_invalid(void)
{
    SimulationResult lru = {0}, clock = {0};
    int refs[] = {1, 2, 3};
    SimulationInput bad = {.num_frames = 0, .references = refs, .num_references = 3};

    assert(simulator_run(NULL, &lru, &clock) == SIM_ERROR);
    assert(simulator_run(&bad, &lru, &clock) == SIM_ERROR);
    assert(lru.steps == NULL && clock.steps == NULL);
    puts("OK: entradas invalidas");
}

static void test_rates(void)
{
    SimulationResult empty = {0};
    assert(simulator_hit_rate(&empty) == 0.0);
    assert(simulator_fault_rate(NULL) == 0.0);
    puts("OK: porcentajes con resultado vacio");
}

int main(void)
{
    test_scenarios();
    test_files();
    test_invalid();
    test_rates();
    puts("Todas las pruebas del simulador finalizaron correctamente.");
    return 0;
}
