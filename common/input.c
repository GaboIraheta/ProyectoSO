#include "input.h"
#include "compat.h"

#include <errno.h>
#include <limits.h>
#include <stdint.h>

#define DELIMITERS " \t\r\n,;"

static int parse_int(const char *token, long *out)
{
    if (token == NULL || out == NULL || *token == '\0') {
        return SIM_ERROR;
    }

    char *end = NULL;

    errno = 0;

    long value = strtol(token, &end, 10);

    if (errno == ERANGE ||
        end == token ||
        *end != '\0') {

        return SIM_ERROR;
    }

    *out = value;

    return SIM_OK;
}

static bool is_blank_or_comment(const char *line)
{
    if (line == NULL) {
        return true;
    }

    while (*line == ' ' ||
           *line == '\t' ||
           *line == '\r') {

        ++line;
    }

    return *line == '\0' ||
           *line == '\n' ||
           *line == '#';
}

static int append_reference(
    SimulationInput *input,
    int page,
    int *capacity)
{
    if (input == NULL ||
        capacity == NULL ||
        input->num_references >= MAX_REFERENCES) {

        fprintf(stderr,
                "Error: limite de referencias alcanzado (%d).\n",
                MAX_REFERENCES);

        return SIM_ERROR;
    }

    if (input->num_references == *capacity) {

        int new_capacity = (*capacity == 0)
                           ? 16
                           : *capacity * 2;

        if (new_capacity > MAX_REFERENCES) {
            new_capacity = MAX_REFERENCES;
        }

        if (new_capacity <= *capacity ||
            (size_t)new_capacity > SIZE_MAX / sizeof(int)) {

            return SIM_ERROR;
        }

        int *tmp = realloc(
            input->references,
            (size_t)new_capacity * sizeof *tmp
        );

        if (tmp == NULL) {
            fprintf(stderr,
                    "Error: no hay memoria para las referencias.\n");

            return SIM_ERROR;
        }

        input->references = tmp;
        *capacity = new_capacity;
    }

    input->references[input->num_references++] = page;

    return SIM_OK;
}

static int parse_frames_line(
    char *line,
    int *num_frames)
{
    if (line == NULL || num_frames == NULL) {
        return SIM_ERROR;
    }

    char *token = strtok(line, DELIMITERS);

    long value = 0;

    if (token == NULL ||
        parse_int(token, &value) != SIM_OK) {

        fprintf(stderr,
                "Error: numero de marcos invalido.\n");

        return SIM_ERROR;
    }

    if (strtok(NULL, DELIMITERS) != NULL) {

        fprintf(stderr,
                "Error: indique un solo numero de marcos.\n");

        return SIM_ERROR;
    }

    if (value < MIN_FRAMES || value > MAX_FRAMES) {

        fprintf(stderr,
                "Error: los marcos deben estar entre %d y %d.\n",
                MIN_FRAMES,
                MAX_FRAMES);

        return SIM_ERROR;
    }

    *num_frames = (int)value;

    return SIM_OK;
}

static int parse_reference_line(
    char *line,
    SimulationInput *input,
    int *capacity)
{
    if (line == NULL ||
        input == NULL ||
        capacity == NULL) {

        return SIM_ERROR;
    }

    for (char *token = strtok(line, DELIMITERS);
         token != NULL;
         token = strtok(NULL, DELIMITERS)) {

        if (token[0] == '#') {
            break;
        }

        long value = 0;

        if (parse_int(token, &value) != SIM_OK ||
            value < 0 ||
            value > MAX_PAGE_NUMBER) {

            fprintf(stderr,
                    "Error: pagina invalida '%s' "
                    "(referencia #%d).\n",
                    token,
                    input->num_references + 1);

            return SIM_ERROR;
        }

        if (append_reference(
                input,
                (int)value,
                capacity) != SIM_OK) {

            return SIM_ERROR;
        }
    }

    return SIM_OK;
}

void input_init(SimulationInput *input)
{
    if (input != NULL) {
        input->num_frames = 0;
        input->references = NULL;
        input->num_references = 0;
    }
}

void input_free(SimulationInput *input)
{
    if (input != NULL) {
        free(input->references);
        input_init(input);
    }
}

int input_validate(const SimulationInput *input)
{
    if (input == NULL ||
        input->num_frames < MIN_FRAMES ||
        input->num_frames > MAX_FRAMES ||
        input->references == NULL ||
        input->num_references <= 0 ||
        input->num_references > MAX_REFERENCES) {

        fprintf(stderr,
                "Error: entrada incompleta o fuera de limites.\n");

        return SIM_ERROR;
    }

    for (int i = 0; i < input->num_references; ++i) {

        if (input->references[i] < 0 ||
            input->references[i] > MAX_PAGE_NUMBER) {

            fprintf(stderr,
                    "Error: pagina invalida en referencia #%d.\n",
                    i + 1);

            return SIM_ERROR;
        }
    }

    return SIM_OK;
}

int input_read_from_file(
    const char *path,
    SimulationInput *input)
{
    if (path == NULL || input == NULL) {
        return SIM_ERROR;
    }

    if (input->references != NULL ||
        input->num_references != 0 ||
        input->num_frames != 0) {

        fprintf(stderr,
                "Error: libere la entrada anterior "
                "antes de leer otra.\n");

        return SIM_ERROR;
    }

    FILE *file = fopen(path, "r");

    if (file == NULL) {
        perror("Error al abrir el archivo de entrada");
        return SIM_ERROR;
    }

    char *line = NULL;
    size_t line_size = 0;

    int capacity = 0;
    bool frames_read = false;
    int status = SIM_OK;

    while (compat_getline(&line, &line_size, file) != -1) {

        if (is_blank_or_comment(line)) {
            continue;
        }

        if (!frames_read) {

            status = parse_frames_line(
                line,
                &input->num_frames
            );

            if (status == SIM_OK) {
                frames_read = true;
            }

        } else {

            status = parse_reference_line(
                line,
                input,
                &capacity
            );
        }

        if (status != SIM_OK) {
            break;
        }
    }

    if (ferror(file)) {

        perror("Error al leer el archivo");

        status = SIM_ERROR;
    }

    free(line);

    if (fclose(file) != 0) {

        perror("Error al cerrar el archivo");

        status = SIM_ERROR;
    }

    if (!frames_read) {

        fprintf(stderr,
                "Error: no se encontro el numero de marcos.\n");

        status = SIM_ERROR;
    }

    if (status == SIM_OK) {
        status = input_validate(input);
    }

    if (status != SIM_OK) {
        input_free(input);
    }

    return status;
}

int input_read_interactive(SimulationInput *input)
{
    if (input == NULL) {
        return SIM_ERROR;
    }

    if (input->references != NULL ||
        input->num_references != 0 ||
        input->num_frames != 0) {

        fprintf(stderr,
                "Error: libere la entrada anterior "
                "antes de leer otra.\n");

        return SIM_ERROR;
    }

    char *line = NULL;
    size_t line_size = 0;

    int capacity = 0;

    while (true) {

        printf(
            "Numero de marcos fisicos (%d-%d): ",
            MIN_FRAMES,
            MAX_FRAMES
        );

        fflush(stdout);

        if (compat_getline(&line, &line_size, stdin) == -1) {

            fprintf(stderr,
                    "Error: fin de entrada al leer marcos.\n");

            free(line);
            input_free(input);

            return SIM_ERROR;
        }

        if (parse_frames_line(
                line,
                &input->num_frames) == SIM_OK) {

            break;
        }
    }

    while (true) {

        printf(
            "Secuencia de referencias "
            "(espacios, comas o punto y coma): "
        );

        fflush(stdout);

        if (compat_getline(&line, &line_size, stdin) == -1) {

            fprintf(stderr,
                    "Error: fin de entrada al leer referencias.\n");

            free(line);
            input_free(input);

            return SIM_ERROR;
        }

        input->num_references = 0;

        int status = parse_reference_line(
            line,
            input,
            &capacity
        );

        if (status == SIM_OK &&
            input->num_references > 0) {

            break;
        }

        if (status == SIM_OK) {

            fprintf(stderr,
                    "Error: ingrese al menos una referencia.\n");
        }
    }

    free(line);

    int status = input_validate(input);

    if (status != SIM_OK) {
        input_free(input);
    }

    return status;
}

void input_print(const SimulationInput *input)
{
    if (input_validate(input) != SIM_OK) {
        return;
    }

    printf("\n=== Datos de entrada ===\n");

    printf(
        "Marcos fisicos       : %d\n",
        input->num_frames
    );

    printf(
        "Total de referencias : %d\n",
        input->num_references
    );

    printf("Secuencia            : ");

    for (int i = 0; i < input->num_references; ++i) {

        printf(
            "%d%s",
            input->references[i],
            i + 1 < input->num_references ? " " : "\n"
        );
    }
}