/* Standard includes. */
//#include <stdio.h>

/* Kernel includes. */
#include "mmio.h"
#include "delay.h"
#include "arch_sleep.h"
/* cvitek includes. */
#include "printf.h"
#include "rtos_cmdqu.h"
#include "cvi_mailbox.h"
#include "intr_conf.h"
#include "top_reg.h"
#include "memmap.h"
#include "comm.h"
#include "cvi_spinlock.h"
#include "pedestal_function.h"
#include "csr.h"
/* Milk-V Duo */
#include "milkv_duo_io.h"
#include <string.h>

#define __DEBUG__
#ifdef __DEBUG__
#define debug_printf printf
#else
#define debug_printf(...)
#endif

/****************************************************************************
 * Function prototypes
 ****************************************************************************/
void prvQueueISR(void);
void prvCmdQuRunTask(void);


/****************************************************************************
 * Global parameters
 ****************************************************************************/

/****************************************************************************
 * Function definitions
 ****************************************************************************/

#define SLEEP_WAIT_BOOT() __asm__ __volatile__("wfi");

int send_test_cmdqu_irq_handler(cmdqu_t* cmdq, void* data)
{
	(void)cmdq;
	(void)data;
	printf("=================send_test_cmdqu====================\n");
	printf("cmdq->ip_id =%d\n", cmdq->ip_id);
	printf("cmdq->cmd_id =%d\n", cmdq->cmd_id);
	printf("cmdq->param_ptr =%x\n", cmdq->param_ptr);
	printf("cmdq->block =%d\n", cmdq->block);
	printf("cmdq->linux_valid =%d\n", cmdq->resv.valid.linux_valid);
	printf("cmdq->rtos_valid =%x\n", cmdq->resv.valid.rtos_valid);
	printf("===================================================\n");
	return 0;
}

#define STATE_INIT 		0x176358
#define STATE_BOOT_RCVHF	0x453577
#define STATE_BOOT_DONE		0x234124

int boot_state = STATE_INIT;
uint64_t boot_addr = 0;
uint8_t  last_package_id = 0;

int boot_cmdqu_handler(cmdqu_t* cmdq, void* data)
{
	(void)data;
	uint8_t 	boot_package_id = cmdq->ip_id;
	uint32_t 	half_addr 		= cmdq->param_ptr;
	if(boot_state == STATE_BOOT_DONE)
	{
		printf("already has boot addr !\n");
		return 0;
	}
	else if(boot_state == STATE_INIT)
	{
		last_package_id = boot_package_id;
		boot_addr = (uint64_t)half_addr;
		boot_state = STATE_BOOT_RCVHF;
	}
	else if(boot_state == STATE_BOOT_RCVHF)
	{
		if(last_package_id - 1 == boot_package_id)
		{
			boot_state = STATE_BOOT_DONE;
			boot_addr = (((boot_addr & 0xffffffffull) << 32) | ((uint64_t)(half_addr) & 0xffffffffull));
		}
		else if(last_package_id + 1 == boot_package_id)
		{
			boot_state = STATE_BOOT_DONE;
			boot_addr = ((boot_addr & 0xffffffffull) | (((uint64_t)(half_addr) & 0xffffffffull) << 32));
		}
		else 
		{
			boot_state = STATE_BOOT_RCVHF;
			last_package_id = boot_package_id;
			boot_addr = (uint64_t)half_addr;
		}
	}
	else 
	{
		printf("unkown boot state!\n");
		return -1;
	}
	return 0;
}

#define IRQ_NUM_MAX		 128
#define IRQ_INTERRUPT_START 	 16
int  milkv_cpu_shut_prepare(unsigned int* irqns, int num)
{
	write_csr(mie, 0);
        write_csr(mip, 0);
	clear_csr(mstatus, MSTATUS_MIE);
	for(int i = 0;i<num;i++)
		disable_irq(irqns[i]);
	boot_state = STATE_INIT;
	boot_addr = 0;
	last_package_id = 0;
	printf("milkv_cpu_shut_prepare done !\n");
	return 0;
}

extern void uni_shutdown(void);
void milkv_cpu_shutdown(void)
{
	uni_shutdown();
}

void pre_boot_data_clear(void)
{
	write_csr(mie, 0);
	write_csr(mip, 0);
	clear_csr(mstatus, MSTATUS_MIE);
	request_cmdqu_irq(CMDQU_SEND_TEST, NULL, NULL);
	request_cmdqu_irq(CMDQU_MCS_BOOT, NULL, NULL);
	clear_irq(MBOX_INT_C906_2ND);
	return;	
}

typedef void (*boot_func)(struct pedestal_operation* ops);

struct pedestal_operation milkvduo_func = {
    .cpu_shut_prepare = milkv_cpu_shut_prepare,
    .cpu_shutdown = milkv_cpu_shutdown,
}; 
void main_shutdown(void)
{
	uart_init();
	printf("\nrebooting!\n");
	int err;
	int ret;
	ret = request_irq(MBOX_INT_C906_2ND, prvQueueISR, 0, "mailbox", (void *)0);
        if(ret != 0)
        {
                printf("error request_irq for mailbox\n");
                err++;
        }
        ret = request_cmdqu_irq(CMDQU_SEND_TEST, send_test_cmdqu_irq_handler, NULL);
        if(ret != 0)
        {
                printf("error hook cmdqu irq handler!\n");
                err++;
        }
        ret = request_cmdqu_irq(CMDQU_MCS_BOOT, boot_cmdqu_handler, NULL);
        if(ret != 0)
        {
                printf("error request mcs_boot package handler!\n");
                err++;
        }
        if(err)
                while(1) SLEEP_WAIT_BOOT();
        prvCmdQuRunTask();	
}

void main_cvirtos(void)
{
	int err;
	int ret;
	printf("create cvi task\n");
	err = 0;
	ret = request_irq(MBOX_INT_C906_2ND, prvQueueISR, 0, "mailbox", (void *)0);
	if(ret != 0)
	{
		printf("error request_irq for mailbox\n");
		err++;
	}
	ret = request_cmdqu_irq(CMDQU_SEND_TEST, send_test_cmdqu_irq_handler, NULL);
	if(ret != 0)
	{
		printf("error hook cmdqu irq handler!\n");
		err++;
	}
	ret = request_cmdqu_irq(CMDQU_MCS_BOOT, boot_cmdqu_handler, NULL);
	if(ret != 0)
	{
		printf("error request mcs_boot package handler!\n");
		err++;
	}
	if(err)
		while(1) SLEEP_WAIT_BOOT();
	prvCmdQuRunTask();
}

void prvCmdQuRunTask(void)
{
	printf("prvCmdQuRunTask run\n");
	while(1)
	{
		SLEEP_WAIT_BOOT();
		if(boot_state == STATE_BOOT_DONE)
		{
			printf("boot_addr: %p\n", boot_addr);
			pre_boot_data_clear();
			struct pedestal_operation* boot_parameter = &milkvduo_func;
			((boot_func)boot_addr)(boot_parameter);
			printf("boot failed!\n");
			while(1) __asm__ __volatile__("wfi");
		}
	}
}

void prvQueueISR(void)
{
	printf("prvQueueISR\n");
	extern void cmdqu_intr(void);
	cmdqu_intr();
}
