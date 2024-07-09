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


int send_test_cmdqu_irq_handler(cmdqu_t* cmdq, void* data)
{
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

void main_cvirtos(void)
{
	printf("create cvi task\n");
	/* Start the tasks and timer running. */
	request_irq(MBOX_INT_C906_2ND, prvQueueISR, 0, "mailbox", (void *)0);
	int ret = request_cmdqu_irq(CMDQU_SEND_TEST, send_test_cmdqu_irq_handler, NULL);
	if(ret != 0)
		printf("error hook cmdqu irq handler!\n");
	prvCmdQuRunTask();
}
int times;
void prvCmdQuRunTask(void)
{
	times = 0;
	printf("prvCmdQuRunTask run\n");
	while(1)
	{
		__asm__ __volatile__("wfi");
		printf("wake up from sleep!!!\n");
		if(times == 8)
		{
			printf("ohh!\n");
			arch_usleep(5000000);
			cmdqu_t test_cmd;
			test_cmd.ip_id = 0;
			test_cmd.cmd_id = CMDQU_SEND_TEST;
			test_cmd.block = 0;
			test_cmd.resv.mstime = 0;
			test_cmd.param_ptr = 0;
			int ret = rtos_cmdqu_send(&test_cmd);
			if(ret)
				printf("rtos_cmdqu_send error!\n");
			arch_usleep(1000000);
			test_cmd.ip_id = 0x23;
			test_cmd.cmd_id = CMDQU_MCS_BOOT;
			test_cmd.block = 1;
			test_cmd.resv.mstime = 0x2;
			test_cmd.param_ptr = 3;
			ret = rtos_cmdqu_send(&test_cmd);
			if(ret)
				printf("rtos_cmdqu_send error!\n");
			arch_usleep(1000000);
			test_cmd.ip_id = 0x12;
			test_cmd.cmd_id = 123;
			test_cmd.block = 1;
			test_cmd.resv.mstime = 0x0;
			test_cmd.param_ptr = 1;
			ret = rtos_cmdqu_send(&test_cmd);
			if(ret)
				printf("rtos_cmdqu_send error!\n");
			arch_usleep(1000000);
			test_cmd.ip_id = 0x0;
			test_cmd.cmd_id = CMDQU_SEND_TEST;
			test_cmd.block = 0;
			test_cmd.resv.mstime = 0x0;
			test_cmd.param_ptr = 0;
			for(int i=0;i<10;i++)
			{
				rtos_cmdqu_send(&test_cmd);
			}
		}
	}
}

void prvQueueISR(void)
{
	times++;
	printf("prvQueueISR\n");
	extern void cmdqu_intr(void);
	cmdqu_intr();
}
