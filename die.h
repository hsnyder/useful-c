#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

static void _Noreturn
#if defined(__clang__) || defined(__GNUC__)
__attribute__ ((format (printf, 1, 2)))
#endif
die(char *fmt, ...)
{
    int e = errno;
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    if (e!= 0) fprintf(stderr, " (errno %d: %s)", e, strerror(e));
    fputc('\n', stderr);
    exit(EXIT_FAILURE);
}


static void 
#if defined(__clang__) || defined(__GNUC__)
__attribute__ ((format (printf, 1, 2)))
#endif
warn(char *fmt, ...)
{
    int e = errno;
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    if (e!= 0) fprintf(stderr, " (errno %d: %s)", e, strerror(e));
    fputc('\n', stderr);
}
