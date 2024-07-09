#ifndef __RTOS_COMMAND_QUEUE__
#define __RTOS_COMMAND_QUEUE__

#include <linux/kernel.h>

struct valid_t {
	unsigned char linux_valid;
	unsigned char rtos_valid;
} __attribute__((packed));

typedef union resv_t {
	struct valid_t valid;
	unsigned short mstime; // 0 : noblock, -1 : block infinite
} resv_t;

typedef struct cmdqu_t cmdqu_t;

/*
 * 	cmdqu_t
 *		cmd_id : package function id
 *			.e.g. 	User use "request_cmdqu_irq(CMDQU_MCS_BOOT,..,..)" to register the callback for CMDQU_MCS_BOOT 'port'
 *			 		when other cpu send the CMDQU_MCS_BOOT type package to self-cpu, the callback function works
 *		ip_id  : reserved, can be defined for the specific function type package
 *		block  : reserved, can be defined for the specific function type package
 *		resv.valid:	decribe the transport direction , linux to rtos  or rtos to linux 
 *		param_ptr : private data for specific type package
 * 			.e.g. 	CMDQU_MCS_BOOT type package will use to represent "rtos boot addr", cause it is u32, not u64,
 * 					so CMDQU_MCS_BOOT type package will send twice
 */
/* cmdqu size should be 8 bytes because of mailbox buffer size */
struct cmdqu_t {
	unsigned char ip_id;
	unsigned char cmd_id : 7;
	unsigned char block : 1;
	union resv_t resv;
	unsigned int  param_ptr;
} __attribute__((packed)) __attribute__((aligned(0x8)));


#define MAX_CMD_NUM	 128
/* keep those commands for ioctl system used */
/* cmd type don't more than 128!!!!!*/
enum SYSTEM_CMD_TYPE {
	CMDQU_SEND_TEST 	= 0	,
	CMDQU_DUO_TEST			,
	CMDQU_MCS_BOOT			,
	CMDQU_MCS_COMMUNICATE	,
	CMDQU_SYSTEM_BUTT		,
};

typedef int (*cmdqu_irq_handler)(cmdqu_t* cmdq, void* data);

/*
 *	Description:
 *		send on package "cmdq" to other cpu
 *	Note:
 * 	1.	now, for rtos(C906L) can only send to Linux(C906B)
 * 		for Linux(C906B) can only send to rtos(C906L)
 * 		so there is no one parameter called "cpuid"
 *  2.	This function can ensure that the cmdq is accepted by target core,
 * 		accepted and call your registered callback function
 *  3.  This function not always return 0(success), a negetive number represent occur some fault
 */
extern int rtos_cmdqu_send(cmdqu_t* cmdq);

extern int request_cmdqu_irq(enum SYSTEM_CMD_TYPE, cmdqu_irq_handler cmdqu_irq_func, void* data);

#endif  // end of __RTOS_COMMAND_QUEUE__
