#include "csr.h"
#include "arch_time.h"
#include "irq.h"
#include "printf.h"
#include "types.h"
#define OS_IS_INTERUPT(mcause)     (mcause & 0x8000000000000000ull)
#define OS_IS_EXCEPTION(mcause)    (~(OS_IS_INTERUPT))
#define OS_IS_TICK_INT(mcause)     (mcause == 0x8000000000000007ull)
#define OS_IS_SOFT_INT(mcause)     (mcause == 0x8000000000000003ull)
#define OS_IS_EXT_INT(mcause)      (mcause == 0x800000000000000bull)
#define OS_IS_TRAP_USER(mcause)    (mcause == 0x000000000000000bull)

extern void do_irq(void);

void trap_entry(uintptr_t sp)
{
   int panic = 0;
   extern char _stack_end_end[];
   extern char _stack_top[];
   if(!(sp >= (uintptr_t)_stack_end_end && sp < (uintptr_t)_stack_top))
   {
	    printf("stackoverflow! sp: %p\n", sp);
   	    panic = 1;
   }
   printf("trap_entry sp %p\n", sp);
   uintptr_t mcause = read_csr(mcause);
   uintptr_t mepc = read_csr(mepc);
   uintptr_t mstatus = read_csr(mstatus);
   uintptr_t mtval = read_csr(mtval);
   // printf("[UniProton]: in trap!\n");
    if(OS_IS_INTERUPT(mcause)) {
        if(OS_IS_TICK_INT(mcause)) {
	    printf("IN TIME trap!wtf???!\n");
	    panic = 1;
        } else if(OS_IS_SOFT_INT(mcause)) {
	    printf("IN SOFT trap!wtf???!\n");
            panic = 1;
        } else if(OS_IS_EXT_INT(mcause)) {
	    //printf("IN EXIT trap!\n");
	    do_irq();
        }
    } else {
	    panic = 1;
	    printf("hardware exception !!!\n");
	    printf("mcause %p mepc %p mstatus %p mtval %p sp %p\n", mcause, mepc, mstatus, mtval, sp);
    }
    if(panic)
    {
        while(1) __asm__ __volatile__("wfi");
    }
    return;
}
