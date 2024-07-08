#include "csr.h"
#include "arch_time.h"
#include "irq.h"
#include "printf.h"
// milkvduo little pre system init 
extern void pre_system_init(void);

// milkvduo litte post system init
extern void post_system_init(void);

extern void main_cvirtos(void);

extern void rv_trap(void);

int test_data[23];

int data[7] = {0x12, 0x34, 0x56, 0x78, 0x90, 0x23, 0x45 };

int main()
{
    pre_system_init();
    post_system_init();
    int bss_ensure = 1;
    int data_ensure = 1;
    for(int i=0; i<23; i++)
    {
	if(test_data[i] != 0)
	{
		bss_ensure = 0;
		break;
	}
    }
    if(data[0] != 0x12 || data[1] !=0x34 || data[2] != 0x56 || data[3] != 0x78 || data[4] != 0x90 || data[5] != 0x23 || data[6] != 0x45 )
	data_ensure = 0;
    if(bss_ensure != 1)
	    printf(".bss not init as wanted!\n");
    if(data_ensure != 1)
    {
	printf(".data not init as wanted!\n");
    	printf("[0-6]: %x %x %x %x %x %x %x\n", data[0], data[1], data[2], data[3], data[4], data[5], data[6]);
    }
    write_csr(mtvec, rv_trap);
    if(data_ensure != 1 || bss_ensure != 1)
	while(1) __asm__ __volatile__("wfi");
    printf("ensure .bss and .data init as wanted!\n");
    printf("[UniProton]: start to init OsConfig!\n");
    main_cvirtos();
    while(1) __asm__  __volatile__("wfi");
}

