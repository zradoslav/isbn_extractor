#ifndef DEBUG_H
#define DEBUG_H

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

// #define print_err(text) fprintf(stderr, ANSI_COLOR_RED text ANSI_COLOR_RESET)
#define print_err(format, ...) fprintf(stderr, ANSI_COLOR_RED format ANSI_COLOR_RESET, __VA_ARGS__)
// #define print_log(text) fprintf(stderr, ANSI_COLOR_GREEN text ANSI_COLOR_RESET)
#define print_log(format, ...) fprintf(stdout, ANSI_COLOR_GREEN format ANSI_COLOR_RESET, __VA_ARGS__)

#endif
