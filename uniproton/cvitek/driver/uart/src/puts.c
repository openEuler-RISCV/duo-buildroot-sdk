//#include <stdio.h>
#include "uart.h"
#include "printf.h"
#include <stddef.h>
int puts(const char *s)
{
	int count = 0;
	while (*s) {
		if (putchar(*s++) != -1) {
			count++;
		} else {
			count = -1;
			break;
		}
	}

	/* According to the puts(3) manpage, the function should write a
     * trailing newline.
     */
	if ((count != -1) && (putchar('\n') != -1))
		count++;
	else
		count = -1;

	return count;
}
