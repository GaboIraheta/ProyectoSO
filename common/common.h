#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIM_OK 0
#define SIM_ERROR (-1)

#define MIN_FRAMES 1
#define MAX_FRAMES 64
#define MAX_REFERENCES 10000
#define MAX_PAGE_NUMBER 9999

#define EMPTY_FRAME (-1)
#define NO_EVICTION (-1)

typedef struct {
    int num_frames;
    int *references;
    int num_references;
} SimulationInput;

typedef struct {
    int page;
    bool hit;
    int evicted_page;
} StepRecord;

typedef struct {
    const char *algorithm_name;
    int num_frames;
    int num_steps;
    int steps_recorded;
    int hits;
    int faults;
    StepRecord *steps;
    int *frame_history;
} SimulationResult;

static inline int *frames_create(int num_frames)
{
    if (num_frames < MIN_FRAMES || num_frames > MAX_FRAMES) {
        fprintf(stderr, "Error: cantidad de marcos fuera de rango.\n");
        return NULL;
    }

    int *frames = malloc((size_t)num_frames * sizeof *frames);

    if (frames == NULL) {
        fprintf(stderr, "Error: no hay memoria para los marcos.\n");
        return NULL;
    }

    for (int i = 0; i < num_frames; ++i) {
        frames[i] = EMPTY_FRAME;
    }

    return frames;
}

static inline void frames_destroy(int **frames)
{
    if (frames != NULL) {
        free(*frames);
        *frames = NULL;
    }
}

static inline void result_free(SimulationResult *result)
{
    if (result == NULL) {
        return;
    }

    free(result->steps);
    free(result->frame_history);

    memset(result, 0, sizeof *result);
}

static inline int result_init(
    SimulationResult *result,
    const char *name,
    const SimulationInput *input)
{
    if (result == NULL || name == NULL || input == NULL) {
        fprintf(stderr,
                "Error: argumentos nulos al inicializar resultado.\n");
        return SIM_ERROR;
    }

    if (input->num_frames < MIN_FRAMES ||
        input->num_frames > MAX_FRAMES ||
        input->num_references <= 0 ||
        input->num_references > MAX_REFERENCES ||
        input->references == NULL) {

        fprintf(stderr,
                "Error: datos de entrada invalidos para el resultado.\n");
        return SIM_ERROR;
    }

    size_t steps_count = (size_t)input->num_references;
    size_t frames_count = (size_t)input->num_frames;

    if (steps_count > SIZE_MAX / frames_count ||
        steps_count * frames_count > SIZE_MAX / sizeof(int)) {

        fprintf(stderr,
                "Error: tamano del historial fuera de rango.\n");
        return SIM_ERROR;
    }

    if (result->steps != NULL ||
        result->frame_history != NULL) {

        fprintf(stderr,
                "Error: libere el resultado anterior antes de reinicializarlo.\n");
        return SIM_ERROR;
    }

    memset(result, 0, sizeof *result);

    result->algorithm_name = name;
    result->num_frames = input->num_frames;
    result->num_steps = input->num_references;

    result->steps = calloc(
        steps_count,
        sizeof *result->steps
    );

    result->frame_history = malloc(
        steps_count * frames_count *
        sizeof *result->frame_history
    );

    if (result->steps == NULL ||
        result->frame_history == NULL) {

        fprintf(stderr,
                "Error: no hay memoria para el resultado de %s.\n",
                name);

        result_free(result);
        return SIM_ERROR;
    }

    return SIM_OK;
}

static inline int result_record_step(
    SimulationResult *result,
    int page,
    bool hit,
    int evicted_page,
    const int *frames)
{
    if (result == NULL ||
        frames == NULL ||
        result->steps == NULL ||
        result->frame_history == NULL ||
        result->algorithm_name == NULL ||
        result->num_frames < MIN_FRAMES ||
        result->num_frames > MAX_FRAMES ||
        result->num_steps <= 0 ||
        result->steps_recorded < 0 ||
        result->steps_recorded >= result->num_steps ||
        page < 0 ||
        page > MAX_PAGE_NUMBER ||
        (evicted_page != NO_EVICTION &&
         (evicted_page < 0 ||
          evicted_page > MAX_PAGE_NUMBER)) ||
        (hit && evicted_page != NO_EVICTION)) {

        fprintf(stderr, "Error: registro de paso invalido.\n");
        return SIM_ERROR;
    }

    size_t index = (size_t)result->steps_recorded;

    result->steps[index].page = page;
    result->steps[index].hit = hit;
    result->steps[index].evicted_page = evicted_page;

    memcpy(
        &result->frame_history[
            index * (size_t)result->num_frames
        ],
        frames,
        (size_t)result->num_frames * sizeof *frames
    );

    if (hit) {
        ++result->hits;
    } else {
        ++result->faults;
    }

    ++result->steps_recorded;

    return SIM_OK;
}

static inline const int *result_frames_at(
    const SimulationResult *result,
    int step)
{
    if (result == NULL ||
        result->frame_history == NULL ||
        result->num_frames < MIN_FRAMES ||
        result->num_frames > MAX_FRAMES ||
        step < 0 ||
        step >= result->steps_recorded) {

        return NULL;
    }

    return &result->frame_history[
        (size_t)step * (size_t)result->num_frames
    ];
}

#endif