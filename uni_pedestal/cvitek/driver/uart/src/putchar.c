
//#include <stdio.h>
#include "uart.h"
#include "printf.h"
int putchar(int c)
{
	int res;
	if (uart_putc((unsigned char)c) >= 0)
		res = c;
	else
		res = -1;

	return res;
}
