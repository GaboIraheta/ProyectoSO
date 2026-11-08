#include "../lru/lru.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_basic_sequence(void)
{
    int references[] = {7, 0, 1, 2, 0, 3, 0, 4};

    SimulationInput input = {
        .num_frames = 3,
        .references = references,
        .num_references = 8
    };

    SimulationResult result = {0};

    assert(lru_run(&input, &result) == SIM_OK);

    assert(result.num_frames == 3);
    assert(result.num_steps == 8);
    assert(result.steps_recorded == 8);

    assert(result.hits == 2);
    assert(result.faults == 6);

    assert(result.steps[3].evicted_page == 7);
    assert(result.steps[5].evicted_page == 1);
    assert(result.steps[7].evicted_page == 2);

    assert(result.steps[4].hit == true);
    assert(result.steps[6].hit == true);

    const int *frames = result_frames_at(&result, 7);

    assert(frames != NULL);
    assert(frames[0] == 4);
    assert(frames[1] == 0);
    assert(frames[2] == 3);

    result_free(&result);
}

static void test_repeated_references(void)
{
    int references[] = {5, 5, 5, 5, 5};

    SimulationInput input = {
        .num_frames = 3,
        .references = references,
        .num_references = 5
    };

    SimulationResult result = {0};

    assert(lru_run(&input, &result) == SIM_OK);

    assert(result.faults == 1);
    assert(result.hits == 4);

    for (int i = 0; i < result.num_steps; ++i) {
        assert(result.steps[i].evicted_page == NO_EVICTION);
    }

    result_free(&result);
}

static void test_single_frame(void)
{
    int references[] = {1, 2, 3, 2};

    SimulationInput input = {
        .num_frames = 1,
        .references = references,
        .num_references = 4
    };

    SimulationResult result = {0};

    assert(lru_run(&input, &result) == SIM_OK);

    assert(result.faults == 4);
    assert(result.hits == 0);

    assert(result.steps[0].evicted_page == NO_EVICTION);
    assert(result.steps[1].evicted_page == 1);
    assert(result.steps[2].evicted_page == 2);
    assert(result.steps[3].evicted_page == 3);

    const int *frames = result_frames_at(&result, 3);

    assert(frames != NULL);
    assert(frames[0] == 2);

    result_free(&result);
}

static void test_page_zero(void)
{
    int references[] = {0, 1, 0, 2};

    SimulationInput input = {
        .num_frames = 2,
        .references = references,
        .num_references = 4
    };

    SimulationResult result = {0};

    assert(lru_run(&input, &result) == SIM_OK);

    assert(result.faults == 3);
    assert(result.hits == 1);

    assert(result.steps[3].evicted_page == 1);

    result_free(&result);
}

static void test_empty_frames(void)
{
    int references[] = {8};

    SimulationInput input = {
        .num_frames = 3,
        .references = references,
        .num_references = 1
    };

    SimulationResult result = {0};

    assert(lru_run(&input, &result) == SIM_OK);

    const int *frames = result_frames_at(&result, 0);

    assert(frames != NULL);

    assert(frames[0] == 8);
    assert(frames[1] == EMPTY_FRAME);
    assert(frames[2] == EMPTY_FRAME);

    assert(result_frames_at(&result, -1) == NULL);
    assert(result_frames_at(&result, 1) == NULL);

    result_free(&result);
}

static void test_invalid_input(void)
{
    int references[] = {1, 2, 3};

    SimulationInput input = {
        .num_frames = 0,
        .references = references,
        .num_references = 3
    };

    SimulationResult result = {0};

    assert(lru_run(&input, &result) == SIM_ERROR);

    input.num_frames = 3;
    input.references = NULL;

    assert(lru_run(&input, &result) == SIM_ERROR);

    input.references = references;
    input.num_references = 0;

    assert(lru_run(&input, &result) == SIM_ERROR);

    input.num_references = 3;
    references[1] = -5;

    assert(lru_run(&input, &result) == SIM_ERROR);

    assert(result.steps == NULL);
    assert(result.frame_history == NULL);
}

static void test_result_consistency(void)
{
    int references[] = {2, 1, 2, 3, 1, 4, 2};

    SimulationInput input = {
        .num_frames = 3,
        .references = references,
        .num_references = 7
    };

    SimulationResult result = {0};

    assert(lru_run(&input, &result) == SIM_OK);

    assert(result.steps_recorded == input.num_references);

    assert(result.hits + result.faults ==
           input.num_references);

    for (int i = 0; i < result.num_steps; ++i) {
        assert(result.steps[i].page == references[i]);

        const int *frames = result_frames_at(&result, i);

        assert(frames != NULL);

        int occurrences = 0;

        for (int j = 0; j < result.num_frames; ++j) {
            if (frames[j] == references[i]) {
                ++occurrences;
            }
        }

        assert(occurrences == 1);
    }

    result_free(&result);
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

    printf("Todas las pruebas de LRU finalizaron correctamente.\n");

    return 0;
}