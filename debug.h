#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define print_err(format, ...) fprintf(stderr, ANSI_COLOR_RED format ANSI_COLOR_RESET, __VA_ARGS__)
#define print_log(format, ...) fprintf(stdout, ANSI_COLOR_GREEN format ANSI_COLOR_RESET, __VA_ARGS__)

#endif
