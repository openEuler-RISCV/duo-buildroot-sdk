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

int _bss_test[23];

int _data_test[7] = {0x12, 0x34, 0x56, 0x78, 0x90, 0x23, 0x45 };

int main()
{
    pre_system_init();
    post_system_init();
    int bss_ensure = 1;
    int data_ensure = 1;
    for(int i=0; i<23; i++)
    {
	if(_bss_test[i] != 0)
	{
		bss_ensure = 0;
		break;
	}
    }
    if(_data_test[0] != 0x12 || _data_test[1] !=0x34 || _data_test[2] != 0x56 || _data_test[3] != 0x78 || _data_test[4] != 0x90 || _data_test[5] != 0x23 || _data_test[6] != 0x45 )
	data_ensure = 0;
    if(bss_ensure != 1)
	    printf(".bss not init as wanted!\n");
    if(data_ensure != 1)
    {
	printf(".data not init as wanted!\n");
    	printf("[0-6]: %x %x %x %x %x %x %x\n", _data_test[0], _data_test[1], _data_test[2], _data_test[3], _data_test[4], _data_test[5], _data_test[6]);
    }
    write_csr(mtvec, rv_trap);
    if(data_ensure != 1 || bss_ensure != 1)
	while(1) __asm__ __volatile__("wfi");
    printf("ensure .bss and .data init as wanted!\n");
    printf("[UniProton]: start to init OsConfig!\n");
    main_cvirtos();
    while(1) __asm__  __volatile__("wfi");
}

