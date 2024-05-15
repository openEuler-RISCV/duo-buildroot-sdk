/* Standard includes. */
//#include <stdio.h>

/* Kernel includes. */
#include "mmio.h"
#include "delay.h"
#include "prt_hwi.h"
#include "prt_task.h"
#include "prt_config.h"
#include "prt_buildef.h"
#include "prt_sem.h"

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

//#define __DEBUG__
#ifdef __DEBUG__
#define debug_printf printf
#else
#define debug_printf(...)
#endif

typedef struct _TASK_CTX_S {
	char        	name[32];
	U32         	stack_size;
	TskPrior      	priority;
	TskEntryFunc 	run_task;
	uintptr_t	task_param[4];
	TskHandle	task_pid;
	SemHandle	sleep_mutex;
} TASK_CTX_S;

/****************************************************************************
 * Function prototypes
 ****************************************************************************/
void prvQueueISR(HwiArg intr_number);
void prvCmdQuRunTask(uintptr_t param_1, uintptr_t param_2, uintptr_t param3, uintptr_t param4);
void test_thread(uintptr_t param_1, uintptr_t param_2, uintptr_t param3, uintptr_t param4);


/****************************************************************************
 * Global parameters
 ****************************************************************************/
TASK_CTX_S gTaskCtx[2] = {
	{
		.name = "CMDQU",
		.stack_size = OS_TSK_DEFAULT_STACK_SIZE,
		.priority = OS_TSK_PRIORITY_LOWEST - 5,
		.run_task = prvCmdQuRunTask,
		.task_param = {(uintptr_t)(&gTaskCtx[0]),0,0,0},
		.task_pid = -1,
		.sleep_mutex = -1,
	},
	{
		.name = "TST_THREAD",
		.stack_size = OS_TSK_DEFAULT_STACK_SIZE,
		.priority = OS_TSK_PRIORITY_LOWEST - 1,
		.run_task = test_thread,
		.task_param = {0,0,0,0},
		.task_pid = -1,
		.sleep_mutex = -1,
	},

};

static cmdqu_t rtos_cmdq_cp;

/* mailbox parameters */
volatile struct mailbox_set_register *mbox_reg;
volatile struct mailbox_done_register *mbox_done_reg;
volatile unsigned long *mailbox_context; // mailbox buffer context is 64 Bytess

/****************************************************************************
 * Function definitions
 ****************************************************************************/
DEFINE_CVI_SPINLOCK(mailbox_lock, SPIN_MBOX);

void main_create_tasks(void)
{
	U8 i = 0;
	for (; i < 2; i++) {
		U32 ret, ret_mtx;
		struct TskInitParam param;
		param.taskEntry = gTaskCtx[i].run_task;
		param.taskPrio = gTaskCtx[i].priority;
		param.stackSize = gTaskCtx[i].stack_size;
		param.name = (char*)gTaskCtx[i].name;
		param.stackAddr = 0;
		param.args[0] = (gTaskCtx[i].task_param)[0];
		param.args[1] = (gTaskCtx[i].task_param)[1];
		param.args[2] = (gTaskCtx[i].task_param)[2];
		param.args[3] = (gTaskCtx[i].task_param)[3];
		ret = PRT_TaskCreate(&(gTaskCtx[i].task_pid), &param);
		ret_mtx = PRT_SemCreate(0, &(gTaskCtx[i].sleep_mutex));
		if(ret != OS_OK || ret_mtx != OS_OK) {
			printf("error create rtoscmd thread ret-%p ret_mtx-%p\n", ret, ret_mtx);
			printf("[UniProton] : I'm going to sleep!\n");
			while(1) {
				asm volatile("wfi");
			}
		}
                printf("gTaskCtx[%d].task_pid : %d\n",i, gTaskCtx[i].task_pid);
		printf("gTaskCtx[%d].sleep_mutex : %d\n",i, gTaskCtx[i].sleep_mutex);
		PRT_TaskResume(gTaskCtx[i].task_pid);
	}
}

void main_cvirtos(void)
{
	printf("create cvi task\n");
	U32 ret;
	/* Start the tasks and timer running. */
	//request_irq(MBOX_INT_C906_2ND, prvQueueISR, 0, "mailbox", (void *)0);
	ret = PRT_HwiSetAttr(MBOX_INT_C906_2ND, 2, OS_HWI_MODE_ENGROSS);
	if(ret != OS_OK ) {
		printf("error  hwi attr set MBOX : %p\n", ret);
		printf("[UniProton] : I'm going to sleep!\n");
		while(1) {
			asm volatile("wfi");
		}
	}
	ret = PRT_HwiCreate(MBOX_INT_C906_2ND, prvQueueISR, 0);
	if(ret != OS_OK ) {
		printf("error create hwi MBOX : %p\n", ret);
		printf("[UniProton] : I'm going to sleep!\n");
		while(1) {
			asm volatile("wfi");
		}
	}
	ret = PRT_HwiEnable(MBOX_INT_C906_2ND);
	if(ret != OS_OK ) {
		printf("error  hwi enable MBOX : %p\n", ret);
		printf("[UniProton] : I'm going to sleep!\n");
		while(1) {
			asm volatile("wfi");
		}
	}
	printf("gTaskCtx[0].task_pid : %d \n",gTaskCtx[0].task_pid);
	printf("gTaskCtx[0].sleep_mutex : %d \n",gTaskCtx[0].sleep_mutex);
	main_create_tasks();
    	printf("cvi task end\n");
}

void prvCmdQuRunTask(uintptr_t param_1, uintptr_t param_2, uintptr_t param_3, uintptr_t param_4)
{
	cmdqu_t rtos_cmdq;
	cmdqu_t *cmdq;
	cmdqu_t *rtos_cmdqu_t;
	static int stop_ip = 0;
	int ret = 0;
	int flags;
	int valid;
	int send_to_cpu = SEND_TO_CPU1;

	unsigned int reg_base = MAILBOX_REG_BASE;

	/* to compatible code with linux side */
	cmdq = &rtos_cmdq;
	mbox_reg = (struct mailbox_set_register *) reg_base;
	mbox_done_reg = (struct mailbox_done_register *) (reg_base + 2);
	mailbox_context = (unsigned long *) (MAILBOX_REG_BUFF);

	cvi_spinlock_init();
	printf("prvCmdQuRunTask run\n");

	for (;;) {
		PRT_SemPend(gTaskCtx[0].sleep_mutex,OS_WAIT_FOREVER);
		memcpy((void *)(&rtos_cmdq),(const void*)(&rtos_cmdq_cp),sizeof(rtos_cmdq_cp));
		printf("get mutex prvCmd!\n");
		switch (rtos_cmdq.cmd_id) {
			case CMD_TEST_A:
				//do something
				//send to C906B
				rtos_cmdq.cmd_id = CMD_TEST_A;
				rtos_cmdq.param_ptr = 0x12345678;
				rtos_cmdq.resv.valid.rtos_valid = 1;
				rtos_cmdq.resv.valid.linux_valid = 0;
				printf("recv cmd(%d) from C906B...send [0x%x] to C906B\n", rtos_cmdq.cmd_id, rtos_cmdq.param_ptr);
				goto send_label;
			case CMD_TEST_B:
				//nothing to do
				printf("nothing to do...\n");
				break;
			case CMD_TEST_C:
				rtos_cmdq.cmd_id = CMD_TEST_C;
				rtos_cmdq.param_ptr = 0x55aa;
				rtos_cmdq.resv.valid.rtos_valid = 1;
				rtos_cmdq.resv.valid.linux_valid = 0;
				printf("recv cmd(%d) from C906B...send [0x%x] to C906B\n", rtos_cmdq.cmd_id, rtos_cmdq.param_ptr);
				goto send_label;
			case CMD_DUO_LED:
				rtos_cmdq.cmd_id = CMD_DUO_LED;
				printf("recv cmd(%d) from C906B, param_ptr [0x%x]\n", rtos_cmdq.cmd_id, rtos_cmdq.param_ptr);
				if (rtos_cmdq.param_ptr == DUO_LED_ON) {
					duo_led_control(1);
				} else {
					duo_led_control(0);
				}
				rtos_cmdq.param_ptr = DUO_LED_DONE;
				rtos_cmdq.resv.valid.rtos_valid = 1;
				rtos_cmdq.resv.valid.linux_valid = 0;
				printf("recv cmd(%d) from C906B...send [0x%x] to C906B\n", rtos_cmdq.cmd_id, rtos_cmdq.param_ptr);
				goto send_label;
			default:
send_label:
				/* used to send command to linux*/
				rtos_cmdqu_t = (cmdqu_t *) mailbox_context;

				debug_printf("RTOS_CMDQU_SEND %d\n", send_to_cpu);
				debug_printf("ip_id=%d cmd_id=%d param_ptr=%x\n", cmdq->ip_id, cmdq->cmd_id, (unsigned int)cmdq->param_ptr);
				debug_printf("mailbox_context = %x\n", mailbox_context);
				debug_printf("linux_cmdqu_t = %x\n", rtos_cmdqu_t);
				debug_printf("cmdq->ip_id = %d\n", cmdq->ip_id);
				debug_printf("cmdq->cmd_id = %d\n", cmdq->cmd_id);
				debug_printf("cmdq->block = %d\n", cmdq->block);
				debug_printf("cmdq->para_ptr = %x\n", cmdq->param_ptr);

				drv_spin_lock_irqsave(&mailbox_lock, flags);
				if (flags == MAILBOX_LOCK_FAILED) {
					printf("[%s][%d] drv_spin_lock_irqsave failed! ip_id = %d , cmd_id = %d\n" , cmdq->ip_id , cmdq->cmd_id);
					break;
				}

				for (valid = 0; valid < MAILBOX_MAX_NUM; valid++) {
					if (rtos_cmdqu_t->resv.valid.linux_valid == 0 && rtos_cmdqu_t->resv.valid.rtos_valid == 0) {
						// mailbox buffer context is 4 bytes write access
						int *ptr = (int *)rtos_cmdqu_t;

						cmdq->resv.valid.rtos_valid = 1;
						*ptr = ((cmdq->ip_id << 0) | (cmdq->cmd_id << 8) | (cmdq->block << 15) |
								(cmdq->resv.valid.linux_valid << 16) |
								(cmdq->resv.valid.rtos_valid << 24));
						rtos_cmdqu_t->param_ptr = cmdq->param_ptr;
						debug_printf("rtos_cmdqu_t->linux_valid = %d\n", rtos_cmdqu_t->resv.valid.linux_valid);
						debug_printf("rtos_cmdqu_t->rtos_valid = %d\n", rtos_cmdqu_t->resv.valid.rtos_valid);
						debug_printf("rtos_cmdqu_t->ip_id =%x %d\n", &rtos_cmdqu_t->ip_id, rtos_cmdqu_t->ip_id);
						debug_printf("rtos_cmdqu_t->cmd_id = %d\n", rtos_cmdqu_t->cmd_id);
						debug_printf("rtos_cmdqu_t->block = %d\n", rtos_cmdqu_t->block);
						debug_printf("rtos_cmdqu_t->param_ptr addr=%x %x\n", &rtos_cmdqu_t->param_ptr, rtos_cmdqu_t->param_ptr);
						debug_printf("*ptr = %x\n", *ptr);
						// clear mailbox
						mbox_reg->cpu_mbox_set[send_to_cpu].cpu_mbox_int_clr.mbox_int_clr = (1 << valid);
						// trigger mailbox valid to rtos
						mbox_reg->cpu_mbox_en[send_to_cpu].mbox_info |= (1 << valid);
						mbox_reg->mbox_set.mbox_set = (1 << valid);
						break;
					}
					rtos_cmdqu_t++;
				}
				drv_spin_unlock_irqrestore(&mailbox_lock, flags);
				if (valid >= MAILBOX_MAX_NUM) {
				    printf("No valid mailbox is available\n");
				    return -1;
				}
				break;
		}
	}
}
void test_thread(uintptr_t param_1, uintptr_t param_2, uintptr_t param3, uintptr_t param4)
{
	
	while(1) {
		printf("[UniProton test Thread]: I'm here!\n");
		PRT_TaskDelay(10);
	}
}
void prvQueueISR(HwiArg intr_number)
{
	printf("prvQueueISR\n");
	unsigned char set_val;
	unsigned char valid_val;
	int i;
	cmdqu_t *cmdq;

	set_val = mbox_reg->cpu_mbox_set[RECEIVE_CPU].cpu_mbox_int_int.mbox_int;

	if (set_val) {
		for(i = 0; i < MAILBOX_MAX_NUM; i++) {
			valid_val = set_val  & (1 << i);

			if (valid_val) {
				cmdqu_t rtos_cmdq;
				cmdq = (cmdqu_t *)(mailbox_context) + i;

				debug_printf("mailbox_context =%x\n", mailbox_context);
				debug_printf("sizeof mailbox_context =%x\n", sizeof(cmdqu_t));
				/* mailbox buffer context is send from linux, clear mailbox interrupt */
				mbox_reg->cpu_mbox_set[RECEIVE_CPU].cpu_mbox_int_clr.mbox_int_clr = valid_val;
				// need to disable enable bit
				mbox_reg->cpu_mbox_en[RECEIVE_CPU].mbox_info &= ~valid_val;

				// copy cmdq context (8 bytes) to buffer ASAP
				*((unsigned long *) &rtos_cmdq) = *((unsigned long *)cmdq);
				/* need to clear mailbox interrupt before clear mailbox buffer */
				*((unsigned long*) cmdq) = 0;

				/* mailbox buffer context is send from linux*/
				if (rtos_cmdq.resv.valid.linux_valid == 1) {
					debug_printf("cmdq=%x\n", cmdq);
					debug_printf("cmdq->ip_id =%d\n", rtos_cmdq.ip_id);
					debug_printf("cmdq->cmd_id =%d\n", rtos_cmdq.cmd_id);
					debug_printf("cmdq->param_ptr =%x\n", rtos_cmdq.param_ptr);
					debug_printf("cmdq->block =%x\n", rtos_cmdq.block);
					debug_printf("cmdq->linux_valid =%d\n", rtos_cmdq.resv.valid.linux_valid);
					debug_printf("cmdq->rtos_valid =%x\n", rtos_cmdq.resv.valid.rtos_valid);
					memcpy((void*)(&rtos_cmdq_cp),(const void*)(&rtos_cmdq),sizeof(rtos_cmdq));
					PRT_SemPost(gTaskCtx[0].sleep_mutex);
				} else
					printf("rtos cmdq is not valid %d, ip=%d , cmd=%d\n",
						rtos_cmdq.resv.valid.rtos_valid, rtos_cmdq.ip_id, rtos_cmdq.cmd_id);
			}
		}
	}
}
