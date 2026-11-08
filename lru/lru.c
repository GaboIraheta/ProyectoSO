#include "lru.h"

#include <stdint.h>

typedef struct LRUNode {
    int page;
    int frame_index;

    struct LRUNode *prev;
    struct LRUNode *next;
    struct LRUNode *hash_next;
} LRUNode;

typedef struct {
    int *frames;
    int num_frames;
    int used_frames;

    LRUNode *head;
    LRUNode *tail;

    LRUNode **buckets;
    size_t bucket_count;
} LRUCache;

static int lru_validate_input(const SimulationInput *input)
{
    if (input == NULL ||
        input->num_frames < MIN_FRAMES ||
        input->num_frames > MAX_FRAMES ||
        input->num_references <= 0 ||
        input->num_references > MAX_REFERENCES ||
        input->references == NULL) {

        fprintf(stderr, "Error LRU: entrada invalida.\n");
        return SIM_ERROR;
    }

    for (int i = 0; i < input->num_references; ++i) {
        int page = input->references[i];

        if (page < 0 || page > MAX_PAGE_NUMBER) {
            fprintf(stderr,
                    "Error LRU: pagina invalida en referencia #%d.\n",
                    i + 1);
            return SIM_ERROR;
        }
    }

    return SIM_OK;
}

static size_t hash_index(const LRUCache *cache, int page)
{
    return (size_t)page % cache->bucket_count;
}

static LRUNode *hash_find(
    const LRUCache *cache,
    int page)
{
    size_t index = hash_index(cache, page);

    LRUNode *current = cache->buckets[index];

    while (current != NULL) {
        if (current->page == page) {
            return current;
        }

        current = current->hash_next;
    }

    return NULL;
}

static void hash_insert(
    LRUCache *cache,
    LRUNode *node)
{
    size_t index = hash_index(cache, node->page);

    node->hash_next = cache->buckets[index];
    cache->buckets[index] = node;
}

static void hash_remove(
    LRUCache *cache,
    LRUNode *node)
{
    size_t index = hash_index(cache, node->page);

    LRUNode **current = &cache->buckets[index];

    while (*current != NULL) {
        if (*current == node) {
            *current = node->hash_next;
            node->hash_next = NULL;
            return;
        }

        current = &(*current)->hash_next;
    }
}

static void list_detach(
    LRUCache *cache,
    LRUNode *node)
{
    if (node->prev != NULL) {
        node->prev->next = node->next;
    } else {
        cache->head = node->next;
    }

    if (node->next != NULL) {
        node->next->prev = node->prev;
    } else {
        cache->tail = node->prev;
    }

    node->prev = NULL;
    node->next = NULL;
}

static void list_push_front(
    LRUCache *cache,
    LRUNode *node)
{
    node->prev = NULL;
    node->next = cache->head;

    if (cache->head != NULL) {
        cache->head->prev = node;
    } else {
        cache->tail = node;
    }

    cache->head = node;
}

static void list_move_to_front(
    LRUCache *cache,
    LRUNode *node)
{
    if (cache->head == node) {
        return;
    }

    list_detach(cache, node);
    list_push_front(cache, node);
}

static int cache_init(
    LRUCache *cache,
    int num_frames)
{
    memset(cache, 0, sizeof *cache);

    cache->num_frames = num_frames;

    cache->frames = frames_create(num_frames);

    if (cache->frames == NULL) {
        return SIM_ERROR;
    }

    cache->bucket_count = (size_t)num_frames * 2U + 1U;

    cache->buckets = calloc(
        cache->bucket_count,
        sizeof *cache->buckets
    );

    if (cache->buckets == NULL) {
        fprintf(stderr,
                "Error LRU: no hay memoria para la tabla hash.\n");

        frames_destroy(&cache->frames);
        return SIM_ERROR;
    }

    return SIM_OK;
}

static void cache_destroy(LRUCache *cache)
{
    if (cache == NULL) {
        return;
    }

    LRUNode *current = cache->head;

    while (current != NULL) {
        LRUNode *next = current->next;

        free(current);
        current = next;
    }

    free(cache->buckets);
    frames_destroy(&cache->frames);

    memset(cache, 0, sizeof *cache);
}

static int lru_process_reference(
    LRUCache *cache,
    int page,
    SimulationResult *result)
{
    bool hit = false;
    int evicted_page = NO_EVICTION;

    LRUNode *node = hash_find(cache, page);

    if (node != NULL) {
        hit = true;

        list_move_to_front(cache, node);
    }

    else if (cache->used_frames < cache->num_frames) {

        node = calloc(1, sizeof *node);

        if (node == NULL) {
            fprintf(stderr,
                    "Error LRU: no hay memoria para un nodo.\n");
            return SIM_ERROR;
        }

        int frame_index = cache->used_frames;

        node->page = page;
        node->frame_index = frame_index;

        cache->frames[frame_index] = page;
        ++cache->used_frames;

        list_push_front(cache, node);
        hash_insert(cache, node);
    }

    else {
        node = cache->tail;

        if (node == NULL) {
            fprintf(stderr,
                    "Error LRU: lista inconsistente.\n");
            return SIM_ERROR;
        }

        evicted_page = node->page;

        hash_remove(cache, node);
        list_detach(cache, node);

        node->page = page;

        cache->frames[node->frame_index] = page;

        list_push_front(cache, node);
        hash_insert(cache, node);
    }

    return result_record_step(
        result,
        page,
        hit,
        evicted_page,
        cache->frames
    );
}

int lru_run(
    const SimulationInput *input,
    SimulationResult *result)
{
    if (result == NULL) {
        fprintf(stderr,
                "Error LRU: resultado nulo.\n");
        return SIM_ERROR;
    }

    if (lru_validate_input(input) != SIM_OK) {
        return SIM_ERROR;
    }

    LRUCache cache;

    if (cache_init(&cache, input->num_frames) != SIM_OK) {
        return SIM_ERROR;
    }

    if (result_init(result, "LRU", input) != SIM_OK) {
        cache_destroy(&cache);
        return SIM_ERROR;
    }

    for (int i = 0; i < input->num_references; ++i) {

        int page = input->references[i];

        if (lru_process_reference(
                &cache,
                page,
                result) != SIM_OK) {

            fprintf(stderr,
                    "Error LRU: fallo al procesar "
                    "la referencia #%d.\n",
                    i + 1);

            cache_destroy(&cache);
            result_free(result);

            return SIM_ERROR;
        }
    }

    cache_destroy(&cache);

    if (result->steps_recorded != input->num_references ||
        result->hits + result->faults != input->num_references) {

        fprintf(stderr,
                "Error LRU: estadisticas inconsistentes.\n");

        result_free(result);
        return SIM_ERROR;
    }

    return SIM_OK;
}