//#include <stdint.h>
#include "hal_uart_dw.h"
#include "hal_pinmux.h"
#include <stdint.h>
#include <stdbool.h>
#include <types.h>


void uart_init(void)
{
	int baudrate = 115200;
	int uart_clock = 25 * 1000 * 1000;

	/* set uart to pinmux_uart1 */
	//pinmux_config(PINMUX_UART0);
	hal_uart_init(UART0, baudrate, uart_clock);
	uart_puts("UniProton : uart init done");
}

uint8_t uart_putc(uint8_t ch)
{
	if (ch == '\n') {
		hal_uart_putc('\r');
	}
	hal_uart_putc(ch);
	return ch;
}

void uart_puts(char *str)
{
	if (!str)
		return;

	while (*str) {
		uart_putc(*str++);
	}
}

int uart_getc(void)
{
	return (int)hal_uart_getc();
}

int uart_tstc(void)
{
	return hal_uart_tstc();
}


int uart_put_buff(char *buf)
{
	int flags;
	int count = 0;
	
	while (buf[count]) {
		if (uart_putc(buf[count]) != '\n') {
			count++;
		} else {
			break;
	    }
	}
	return count;
}

