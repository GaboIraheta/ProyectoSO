
// SimulationInput : datos de entrada (marcos + secuencia de referencias).
// StepRecord      : lo que paso en cada referencia (hit/fault, reemplazo).
// SimulationResult: el resultado completo (LRU o Clock).
// Funciones para crear/liberar marcos y resultados.

#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIM_OK     0
#define SIM_ERROR -1

//Limites de validacion
#define MIN_FRAMES        1      
#define MAX_FRAMES        64     
#define MAX_REFERENCES    10000  
#define MAX_PAGE_NUMBER   9999  

// Valores para frame vacio y para casos en los que no se reemplaza ninguna pagina
#define EMPTY_FRAME  -1 
#define NO_EVICTION  -1 


// Datos de entrada

//Entrada
typedef struct {
    int  num_frames;      // Cantidad de marcos                      
    int *references;      // Arreglo dinamico con la secuencia de paginas    
    int  num_references;  // Cuantas referencias hay en el arreglo
} SimulationInput;


typedef struct {
    int  page;          // pagina que se referencio                          
    bool hit;           // true = hit (ya esta en memoria esa pagina), false = fault (no esta en memoria)     
    int  evicted_page;  // pagina reemplazada, o NO_EVICTION si no hubo       
} StepRecord;

//Partes del Resultado de un algoritmo:

//Frame history:
typedef struct {
    const char *algorithm_name;  //"LRU" o "Clock"
    int         num_frames;      // marcos usados
    int         num_steps;       // numero de pasos o referencias
    int         steps_recorded;  
    int         hits;            
    int         faults;          
    StepRecord *steps;           
    int        *frame_history;   
} SimulationResult;

// Funciones compartidas.

// Crea un arreglo de marcos y los marca todos como vacíos.
static inline int *frames_create(int num_frames)
{
    int *frames = malloc((size_t)num_frames * sizeof *frames);
    if (frames == NULL) {
        fprintf(stderr, "Error: no hay memoria para los marcos.\n");
        return NULL;
    }
    for (int i = 0; i < num_frames; i++) {
        frames[i] = EMPTY_FRAME;
    }
    return frames;
}

// Libera los marcos y deja el puntero en NULL para evitar usarlo después.
static inline void frames_destroy(int **frames)
{
    if (frames != NULL) {
        free(*frames);
        *frames = NULL;
    }
}

// Libera la memoria de un resultado y lo deja en ceros. 
static inline void result_free(SimulationResult *result)
{
    if (result == NULL) {
        return;
    }
    free(result->steps);
    free(result->frame_history);
    memset(result, 0, sizeof *result);
}

// Prepara un resultado vacío con espacio para todas las referencias. 
static inline int result_init(SimulationResult *result, const char *name,
                              const SimulationInput *input)
{
    memset(result, 0, sizeof *result);
    result->algorithm_name = name;
    result->num_frames     = input->num_frames;
    result->num_steps      = input->num_references;

    result->steps = calloc((size_t)input->num_references, sizeof *result->steps);
    result->frame_history = malloc((size_t)input->num_references *
                                   (size_t)input->num_frames *
                                   sizeof *result->frame_history);

    if (result->steps == NULL || result->frame_history == NULL) {
        fprintf(stderr, "Error: no hay memoria para el resultado de %s.\n", name);
        result_free(result);
        return SIM_ERROR;
    }
    return SIM_OK;
}


//Registra lo que pasó en una referencia. 
// LRU y Clock deben llamarla una vez CADA vez que se procesa una referencia

static inline int result_record_step(SimulationResult *result, int page, bool hit, int evicted_page, const int *frames)
{
    if (result->steps_recorded >= result->num_steps) {
        fprintf(stderr, "Error: %s registró más pasos de los esperados.\n",
                result->algorithm_name);
        return SIM_ERROR;
    }

    int i = result->steps_recorded;

    result->steps[i].page         = page;
    result->steps[i].hit          = hit;
    result->steps[i].evicted_page = evicted_page;

    /* Copia la "foto" de los marcos en la posición que le toca al paso i. */
    memcpy(&result->frame_history[(size_t)i * (size_t)result->num_frames],
           frames, (size_t)result->num_frames * sizeof *frames);

    if (hit) {
        result->hits++;
    } else {
        result->faults++;
    }
    result->steps_recorded++;
    return SIM_OK;
}

// Devuelve la foto de los marcos en el paso indicado (para imprimirla).
static inline const int *result_frames_at(const SimulationResult *result, int step)
{
    return &result->frame_history[(size_t)step * (size_t)result->num_frames];
}

#endif 
