#include "adapt_libbound.h"


int memset_s(void *dest, size_t destMax, int c, size_t count)
{
	size_t realMax = (count>destMax?destMax:count);
	for(size_t i =0; i<realMax; i++) {
		((char*)dest)[i] = (char)c;
	}
	return EOK;
}

int strncpy_s(char *strDest, size_t destMax, const char *strSrc, size_t count)
{
	size_t real = (count>destMax?destMax:count);
	for(size_t i =0; i<real; i++) {
		if(strSrc[i] == '\0') {
			break;
		}
		strDest[i] = strSrc[i];
	}
	return EOK;
}

int memcpy_s(void *dest, size_t destMax, const void *src, size_t count)
{
	size_t real = (count > destMax?destMax:count);
	for(size_t i =0; i<real; i++) {
		((char*)dest)[i] = ((const char*)src)[i];
	}
	return EOK;
}

