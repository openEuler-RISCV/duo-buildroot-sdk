#ifndef __MALLOC_H___
#define __MALLOC_H__
#include "prt_mem.h"
/* here use uniproton malloc & free function*/
extern void *PRT_MemAlloc(U32 mid, U8 ptNo, U32 size);
extern U32 PRT_MemFree(U32 mid, void *addr);

void *memset(void *dest, int value, unsigned long size);
void *memcpy(void *dest, const void *source, unsigned long size);

/*
 * Replace the normal allocation functions with the versions from
 * <linux/decompress/mm.h>. vfree() needs to support vfree(NULL)
 */
#undef malloc
#undef free
#undef kmalloc
#undef kfree
#undef vmalloc
#undef vfree
#define malloc(size) PRT_MemAlloc(0, 0, size)
#define free(ptr) PRT_MemFree(0, ptr)
#define kmalloc(size, flags) PRT_MemAlloc(0, 0, size)
#define kcalloc(nelem, size, flags) calloc(nelem, size)
#define kzalloc(size, flags) calloc(1, size)
#define devm_kzalloc(nelem, size, flags) calloc(1, size)
#define kfree(ptr) PRT_MemFree(0, ptr)
#define vmalloc(size) PRT_MemAlloc(0, 0, size)
#define vfree(ptr)				\
	do {                        \
		if (ptr != NULL)        \
			PRT_MemFree(0, ptr);     \
	} while (0)

void *calloc(size_t nelem, size_t size);

/* align addr on a size boundary - adjust address up/down if needed */
#define _ALIGN_UP(addr, size)	(((addr)+((size)-1))&(~((typeof(addr))(size)-1)))
#define _ALIGN_DOWN(addr, size)	((addr)&(~((typeof(addr))(size)-1)))

/* align addr on a size boundary - adjust address up if needed */
#define _ALIGN(addr,size)     _ALIGN_UP(addr,size)

/* to align the pointer to the (next) page boundary */
#define PAGE_ALIGN(addr)	_ALIGN(addr, PAGE_SIZE)

#endif // end of __MALLOC_H__
