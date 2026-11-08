#define _POSIX_C_SOURCE 200809L
#include "input.h"
#include <errno.h>

// Caracteres que separan los números
#define DELIMITERS " \t\r\n,;"

// Funciones

// convertir texto a int
static int parse_int(const char *token, long *out)
{
    char *end;
    errno = 0;
    long value = strtol(token, &end, 10);

    if (end == token) {          // No se leyo ningun digito
        return SIM_ERROR;
    }
    if (*end != '\0') {          // hay caracteres de mas
        return SIM_ERROR;
    }
    if (errno == ERANGE) {       // demasiado largo
        return SIM_ERROR;
    }
    *out = value;
    return SIM_OK;
}

// ver si una linea esta vacia o es un comentario
static bool is_blank_or_comment(const char *line)
{
    while (*line == ' ' || *line == '\t') {
        line++;
    }
    return *line == '\0' || *line == '\n' || *line == '\r' || *line == '#';
}

// agregar paginas al arreglo de referencias
static int append_reference(SimulationInput *input, int page, int *capacity)
{
    if (input->num_references >= MAX_REFERENCES) {
        fprintf(stderr, "Error: la secuencia supera el máximo de %d referencias.\n",
                MAX_REFERENCES);
        return SIM_ERROR;
    }

    if (input->num_references == *capacity) {
        int new_capacity = (*capacity == 0) ? 16 : *capacity * 2;
        int *tmp = realloc(input->references, (size_t)new_capacity * sizeof *tmp);
        if (tmp == NULL) {
            fprintf(stderr, "Error: no hay memoria para las referencias.\n");
            return SIM_ERROR;
        }
        input->references = tmp;
        *capacity = new_capacity;
    }

    input->references[input->num_references] = page;
    input->num_references++;
    return SIM_OK;
}

//leer numero de marcos
static int parse_frames_line(char *line, int *num_frames)
{
    char *token = strtok(line, DELIMITERS);
    long value;

    if (token == NULL) {
        fprintf(stderr, "Error: no se indicó el número de marcos.\n");
        return SIM_ERROR;
    }
    if (parse_int(token, &value) != SIM_OK) {
        fprintf(stderr, "Error: '%s' no es un número de marcos válido.\n", token);
        return SIM_ERROR;
    }
    if (strtok(NULL, DELIMITERS) != NULL) {
        fprintf(stderr, "Error: la línea de marcos debe contener un solo número.\n");
        return SIM_ERROR;
    }
    if (value < MIN_FRAMES || value > MAX_FRAMES) {
        fprintf(stderr, "Error: el número de marcos debe estar entre %d y %d (se recibió %ld).\n",
                MIN_FRAMES, MAX_FRAMES, value);
        return SIM_ERROR;
    }

    *num_frames = (int)value;
    return SIM_OK;
}

// separar una linea en numero y los agrega a la secuencia
static int parse_reference_line(char *line, SimulationInput *input, int *capacity)
{
    for (char *token = strtok(line, DELIMITERS);
         token != NULL;
         token = strtok(NULL, DELIMITERS)) {

        long value;
        if (parse_int(token, &value) != SIM_OK) {
            fprintf(stderr, "Error: '%s' no es un número de página válido (referencia #%d).\n",
                    token, input->num_references + 1);
            return SIM_ERROR;
        }
        if (value < 0 || value > MAX_PAGE_NUMBER) {
            fprintf(stderr, "Error: la página %ld está fuera del rango [0, %d] (referencia #%d).\n",
                    value, MAX_PAGE_NUMBER, input->num_references + 1);
            return SIM_ERROR;
        }
        if (append_reference(input, (int)value, capacity) != SIM_OK) {
            return SIM_ERROR;
        }
    }
    return SIM_OK;
}

// Funciones publicas

void input_init(SimulationInput *input)
{
    input->num_frames     = 0;
    input->references     = NULL;
    input->num_references = 0;
}

int input_read_from_file(const char *path, SimulationInput *input)
{
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        perror("Error al abrir el archivo de entrada");
        return SIM_ERROR;
    }

    char  *line        = NULL;  
    size_t line_size   = 0;
    int    capacity    = 0;     
    bool   frames_read = false;
    int    status      = SIM_OK;

    while (getline(&line, &line_size, file) != -1) {
        if (is_blank_or_comment(line)) {
            continue;
        }
        if (!frames_read) {
            status = parse_frames_line(line, &input->num_frames);
            frames_read = true;
        } else {
            status = parse_reference_line(line, input, &capacity);
        }
        if (status != SIM_OK) {
            break;
        }
    }

    free(line);
    fclose(file);

    if (status != SIM_OK) {
        return SIM_ERROR;
    }
    if (!frames_read) {
        fprintf(stderr, "Error: el archivo '%s' no contiene datos.\n", path);
        return SIM_ERROR;
    }
    return input_validate(input);
}

int input_read_interactive(SimulationInput *input)
{
    char  *line      = NULL;
    size_t line_size = 0;
    int    capacity  = 0;

    while (true) {
        printf("Número de marcos físicos (%d-%d): ", MIN_FRAMES, MAX_FRAMES);
        fflush(stdout);
        if (getline(&line, &line_size, stdin) == -1) {   /* Ctrl+D o fin de entrada */
            fprintf(stderr, "\nError: entrada terminada antes de tiempo.\n");
            free(line);
            return SIM_ERROR;
        }
        if (parse_frames_line(line, &input->num_frames) == SIM_OK) {
            break;
        }
    }

    while (true) {
        printf("Secuencia de referencias (separadas por espacios o comas): ");
        fflush(stdout);
        if (getline(&line, &line_size, stdin) == -1) {
            fprintf(stderr, "\nError: entrada terminada antes de tiempo.\n");
            free(line);
            return SIM_ERROR;
        }
        if (parse_reference_line(line, input, &capacity) == SIM_OK &&
            input->num_references > 0) {
            break;
        }
        if (input->num_references == 0) {
            fprintf(stderr, "Error: debe ingresar al menos una referencia.\n");
        }
        
        input->num_references = 0;
    }

    free(line);
    return input_validate(input);
}

int input_validate(const SimulationInput *input)
{
    if (input == NULL) {
        fprintf(stderr, "Error: entrada nula.\n");
        return SIM_ERROR;
    }
    if (input->num_frames < MIN_FRAMES || input->num_frames > MAX_FRAMES) {
        fprintf(stderr, "Error: número de marcos inválido (%d).\n", input->num_frames);
        return SIM_ERROR;
    }
    if (input->references == NULL || input->num_references <= 0) {
        fprintf(stderr, "Error: la secuencia de referencias está vacía.\n");
        return SIM_ERROR;
    }
    if (input->num_references > MAX_REFERENCES) {
        fprintf(stderr, "Error: demasiadas referencias (%d).\n", input->num_references);
        return SIM_ERROR;
    }
    for (int i = 0; i < input->num_references; i++) {
        if (input->references[i] < 0 || input->references[i] > MAX_PAGE_NUMBER) {
            fprintf(stderr, "Error: la referencia #%d (%d) está fuera de rango.\n",
                    i + 1, input->references[i]);
            return SIM_ERROR;
        }
    }
    return SIM_OK;
}

void input_print(const SimulationInput *input)
{
    printf("\n=== Datos de entrada ===\n");
    printf("Marcos físicos       : %d\n", input->num_frames);
    printf("Total de referencias : %d\n", input->num_references);
    printf("Secuencia            : ");
    for (int i = 0; i < input->num_references; i++) {
        printf("%d%s", input->references[i],
               (i < input->num_references - 1) ? " " : "\n");
    }
}

void input_free(SimulationInput *input)
{
    if (input == NULL) {
        return;
    }
    free(input->references);   
    input_init(input);         
}
