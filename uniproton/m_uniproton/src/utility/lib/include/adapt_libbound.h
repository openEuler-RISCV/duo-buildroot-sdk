#ifndef __LIB_ADAPT_H
#define __LIB_ADAPT_H

#include "prt_typedef.h"

#define size_t U32

#define EOK 0

int memset_s(void *dest, size_t destMax, int c, size_t count);

int strncpy_s(char *strDest, size_t destMax, const char *strSrc, size_t count);

int memcpy_s(void *dest, size_t destMax, const void *src, size_t count);

#endif
