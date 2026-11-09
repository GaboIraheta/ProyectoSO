#ifndef COMPAT_H
#define COMPAT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

/* Equivalente portable de getline(): devuelve -1 en EOF o error. */
long compat_getline(char **line, size_t *size, FILE *file);

/* true si stdout es una terminal. */
bool compat_stdout_is_tty(void);

/*
 * Activa los codigos de color ANSI en la consola (necesario en Windows).
 * Devuelve true si la terminal los soporta.
 */
bool compat_enable_ansi(void);

#endif /* COMPAT_H */
