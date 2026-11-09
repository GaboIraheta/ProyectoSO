#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "compat.h"

#include <stdlib.h>

#ifdef _WIN32
#include <io.h>
#include <windows.h>

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#else
#include <unistd.h>
#endif

long compat_getline(char **line, size_t *size, FILE *file)
{
    if (line == NULL || size == NULL || file == NULL) {
        return -1;
    }

    if (*line == NULL || *size == 0) {
        *size = 128;
        *line = malloc(*size);
        if (*line == NULL) {
            *size = 0;
            return -1;
        }
    }

    size_t len = 0;
    int c;

    while ((c = fgetc(file)) != EOF) {
        if (len + 2 > *size) {
            char *tmp = realloc(*line, *size * 2);
            if (tmp == NULL) {
                return -1;
            }
            *line = tmp;
            *size *= 2;
        }

        (*line)[len++] = (char)c;

        if (c == '\n') {
            break;
        }
    }

    if (len == 0) {
        return -1;
    }

    (*line)[len] = '\0';
    return (long)len;
}

bool compat_stdout_is_tty(void)
{
#ifdef _WIN32
    return _isatty(_fileno(stdout)) != 0;
#else
    return isatty(STDOUT_FILENO) != 0;
#endif
}

bool compat_enable_ansi(void)
{
#ifdef _WIN32
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;

    if (out == INVALID_HANDLE_VALUE || !GetConsoleMode(out, &mode)) {
        return false;
    }

    return SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
#else
    return true;
#endif
}
