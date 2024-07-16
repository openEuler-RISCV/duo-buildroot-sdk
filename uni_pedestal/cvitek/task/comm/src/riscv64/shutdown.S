
.global uni_shutdown
.type uni_shutdown, function

.extern rv_trap
.extern main_shutdown

#include "csr.h"

.section .text
uni_shutdown:
	# clear normal registers
	li x1, 0
	li x2, 0
	li x3, 0
	li x4, 0
	li x5, 0
	li x6, 0
	li x7, 0
	li x8, 0
	li x9, 0
	li x10, 0
	li x11, 0
	li x12, 0
	li x13, 0
	li x14, 0
	li x15, 0
	li x16, 0
	li x17, 0
	li x18, 0
	li x19, 0
	li x20, 0
	li x21, 0
	li x22, 0
	li x23, 0
	li x24, 0
	li x25, 0
	li x26, 0
	li x27, 0
	li x28, 0
	li x29, 0
	li x30, 0
	li x31, 0
	
	#reset csr register
	csrw mscratch, zero
	la t0, rv_trap
	csrw mtvec, t0

	#enable MIE, MPIE
	li t0, MSTATUS_MIE | MSTATUS_MPIE
	csrrs t1, mstatus, t0

	#enable MEIE
	li t0, MIP_MEIE
	csrrs t1, mie, t0

	// Primary hart
	la sp, _stack_top
	
	j main_shutdown
err:
	wfi
	j err
	
