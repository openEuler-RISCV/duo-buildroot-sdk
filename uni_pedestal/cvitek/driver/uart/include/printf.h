#ifndef _PRINTF_H
#define _PRINTF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdarg.h>
int printf(const char *fmt, ...);
int puts(const char *s);
int putchar(int c);
int vsnprintf(char *out, size_t n, const char *s, va_list vl); 
int snprintf(char *out, size_t n, const char *s, ...);

#ifdef __cplusplus
}
#endif

#endif /* end of printf*/
