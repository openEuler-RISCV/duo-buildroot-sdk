/*
 * Copyright (c) 2013-2014, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdarg.h>
#include <prt_hwi.h>
#include "uart.h"

/* Choose max of 128 chars for now. */
#define PRINT_BUFFER_SIZE 128

int printf(const char *fmt, ...)
{
	va_list args;
	char buf[PRINT_BUFFER_SIZE];
	int count;
	int pos;
	static int message_id = 0;
	int msg_id_now;
	uintptr_t int_save = PRT_HwiLock();
	msg_id_now = message_id++;
	PRT_HwiRestore(int_save);
	pos = snprintf(buf, sizeof(buf) - 1, "[UniProton %d] :", msg_id_now);
	
	va_start(args, fmt);
	vsnprintf(buf + pos, sizeof(buf) - 1 - pos, fmt, args);
	va_end(args);

	/* Use putchar directly as 'puts()' adds a newline. */
	buf[PRINT_BUFFER_SIZE - 1] = '\0';
	int_save = PRT_HwiLock();
	count = uart_put_buff(buf);
	PRT_HwiRestore(int_save);
	return (count + pos);
}
