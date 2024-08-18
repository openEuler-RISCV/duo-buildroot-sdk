
.section .text
.global rv_trap
.align 4
rv_trap:
	addi sp, sp , -256
     	sd   ra, 248(sp)
    	sd   gp, 240(sp)
    	sd   tp, 232(sp)
   	sd   t0, 224(sp)
    	sd   t1, 216(sp)
    	sd   t2, 208(sp)
    	sd   s0, 200(sp)
    	sd   s1, 192(sp)
    	sd   a0, 184(sp)
    	sd   a1, 176(sp)
   	sd   a2, 168(sp)
    	sd   a3, 160(sp)
    	sd   a4, 152(sp)
    	sd   a5, 144(sp)
    	sd   a6, 136(sp)
    	sd   a7, 128(sp)
    	sd   s2, 120(sp)
    	sd   s3, 112(sp)
    	sd   s4, 104(sp)
    	sd   s5, 96(sp)
    	sd   s6, 88(sp)
    	sd   s7, 80(sp)
    	sd   s8, 72(sp)
    	sd   s9, 64(sp)
    	sd  s10, 56(sp)
    	sd  s11, 48(sp)
    	sd   t3, 40(sp)
    	sd   t4, 32(sp)
    	sd   t5, 24(sp)
    	sd   t6, 16(sp)
    	csrr t0, mstatus
    	sd   t0, 8(sp)
    	csrr t0, mepc
    	sd   t0, 0(sp)
	.extern trap_entry
	add  a0, zero, sp
	call trap_entry
	ld  t1,  0(sp)
    	csrw mepc, t1
    	ld  t1,  8(sp)
    	csrw mstatus, t1
    	ld  t6, 16(sp)
    	ld  t5, 24(sp)
    	ld  t4, 32(sp)
    	ld  t3, 40(sp)
    	ld  s11, 48(sp)
    	ld  s10, 56(sp)
    	ld  s9, 64(sp)
    	ld  s8, 72(sp)
    	ld  s7, 80(sp)
    	ld  s6, 88(sp)
    	ld  s5, 96(sp)
    	ld  s4, 104(sp)
    	ld  s3, 112(sp)
    	ld  s2, 120(sp)
    	ld  a7, 128(sp)
   	ld  a6, 136(sp)
    	ld  a5, 144(sp)
    	ld  a4, 152(sp)
    	ld  a3, 160(sp)
    	ld  a2, 168(sp)
    	ld  a1, 176(sp)
    	ld  a0, 184(sp)
    	ld  s1, 192(sp)
    	ld  s0, 200(sp)
    	ld  t2, 208(sp)
    	ld  t1, 216(sp)
	ld  t0, 224(sp)
    	ld  tp, 232(sp)
    	ld  gp, 240(sp)
    	ld  ra, 248(sp)
    	addi sp, sp, 256
	mret

