#ifndef _DIE_H
#define _DIE_H
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>

static _Noreturn void 
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

#ifdef NDEBUG
#define xassert(cond) do{(void)(cond)}while(0);
#else
#define xassert(cond) if(!(cond)){ die("Assertion failed %s:%i %s", __FILE__, __LINE__, #cond); }
#endif

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
	fflush(stderr);
}

static inline char *
#if defined(__clang__) || defined (__GNUC__)
__attribute__ ((format (printf, 3, 4)))
#endif
fmt_str (int n, char *restrict buffer, const char *restrict fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buffer, n, fmt, ap);
        va_end(ap);
        return buffer;
}

#define NFORMAT(N, fmt, ...) fmt_str(N, (char[N]){0}, (fmt), __VA_ARGS__)

#endif
