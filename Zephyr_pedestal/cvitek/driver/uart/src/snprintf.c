// See LICENSE for license details.

#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include "printf.h"
int vsnprintf(char *out, size_t n, const char *s, va_list vl)
{
	bool format = false;
	bool longarg = false;
	size_t pos = 0;
	int shift_digit = 0;
	for (; *s; s++) {
		if (format) {
			switch (*s) {
			case 'l':
				longarg = true;
				break;
			case 'p':
				longarg = true;
				if (++pos < n)
					out[pos - 1] = '0';
				if (++pos < n)
					out[pos - 1] = 'x';
			case 'x': {
				long num = longarg ? va_arg(vl, long) :
						     va_arg(vl, int);
				for (int i = 2 * (longarg ? sizeof(long) :
							    sizeof(int)) -
					     1;
				     i >= 0; i--) {
					int d = (num >> (4 * i)) & 0xF;
					if (++pos < n)
						out[pos - 1] =
							(d < 10 ? '0' + d :
								  'a' + d - 10);
				}
				longarg = false;
				format = false;
				break;
			}
			case 'd': {
				long num = longarg ? va_arg(vl, long) :
						     va_arg(vl, int);
				if (num < 0) {
					num = -num;
					if (++pos < n)
						out[pos - 1] = '-';
				}
				long digits = 1;
				for (long nn = num; nn /= 10; digits++)
					;
				if (shift_digit) {
					for (int i = shift_digit - 1; i >= 0; i--) {
						if (pos + i + 1 < n) {
							if (i >= shift_digit - digits) {
								out[pos + i] = '0' + (num % 10);
								num /= 10;
							} else
								out[pos + i] = '0';
						}
					}
					pos += shift_digit;
					shift_digit = 0;
				} else {
					for (int i = digits - 1; i >= 0; i--) {
						if (pos + i + 1 < n)
							out[pos + i] = '0' + (num % 10);
						num /= 10;
					}
					pos += digits;
				}
				longarg = false;
				format = false;
				break;
			}
			case 's': {
				const char *s2 = va_arg(vl, const char *);
				while (*s2) {
					if (++pos < n)
						out[pos - 1] = *s2;
					s2++;
				}
				longarg = false;
				format = false;
				break;
			}
			case 'c': {
				if (++pos < n)
					out[pos - 1] = (char)va_arg(vl, int);
				longarg = false;
				format = false;
				break;
			}
			default:
				if(format)
					shift_digit = *s - '0';
				break;
			}
		} else if (*s == '%')
			format = true;
		else if (++pos < n)
			out[pos - 1] = *s;
	}
	if (pos < n)
		out[pos] = 0;
	else if (n)
		out[n - 1] = 0;
	return pos;
}

int snprintf(char *out, size_t n, const char *s, ...)
{
	va_list vl;
	va_start(vl, s);
	int res = vsnprintf(out, n, s, vl);
	va_end(vl);
	return res;
}
