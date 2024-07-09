#include <linux/init.h>
#include <linux/module.h>
#include <linux/irqchip/arm-gic.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/list.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/of_reserved_mem.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/bug.h>

#include "rtos_cmdqu.h"
#include "cvi_mailbox.h"
#include "cvi_spinlock.h"

struct cvi_rtos_cmdqu_device {
	struct device *dev;
};

spinlock_t mailbox_queue_lock;
static __u64  reg_base;
static int mailbox_irq;


static cmdqu_irq_handler g_cmd_handlers[MAX_CMD_NUM];
static void*		 g_private_data[MAX_CMD_NUM];

#define CMDID_VALID(x)  				((x) >= CMDQU_SEND_TEST && (x) < CMDQU_SYSTEM_BUTT)
#define CMDID_INVALID(x)				(!CMDID_VALID(x))
static inline int cmdid_handler_occupied(enum SYSTEM_CMD_TYPE cmdid)
{
	if(CMDID_INVALID(cmdid))
		return -EINVAL;
	if(g_cmd_handlers[cmdid - CMDQU_SEND_TEST] != NULL)
		return 1;
	return 0;
}
static inline int cmdid_handler_free(enum SYSTEM_CMD_TYPE cmdid)
{
	int ret = cmdid_handler_occupied(cmdid);
	if(ret < 0)
		return ret;
	return !ret;
}
//private function
static inline void __set_cmdid_handler(enum SYSTEM_CMD_TYPE cmdid, cmdqu_irq_handler handler, void* data)
{
		g_cmd_handlers[cmdid - CMDQU_SEND_TEST] = handler;
		g_private_data[cmdid - CMDQU_SEND_TEST] = data;
}


int request_cmdqu_irq(enum SYSTEM_CMD_TYPE cmdid, cmdqu_irq_handler cmdqu_irq_func, void* data)
{
	if(CMDID_INVALID(cmdid))
		return -EINVAL;
	__set_cmdid_handler(cmdid, cmdqu_irq_func, data);
	return 0;
}


/* used for callback test*/
static void callback_rtos_irq_handler(int cmd_id, unsigned int ptr, void *dev_id)
{
	pr_info("callback_rtos_hander cmd_id = %x ptr=%x dev_id=%p\n", cmd_id, ptr, dev_id);
}

DEFINE_CVI_SPINLOCK(mailbox_lock, SPIN_MBOX);

irqreturn_t rtos_irq_handler(int irq, void *dev_id)
{
	char set_val;
	int i;
	int flags;
	cmdqu_t *cmdq;

	drv_spin_lock_irqsave(&mailbox_lock, flags);
	if (flags == MAILBOX_LOCK_FAILED) {
		pr_err("drv_spin_lock_irqsave failed!\n");
		//must clear irq?
		return IRQ_HANDLED;
	}
	pr_info("rtos_irq_handler irq=%d\n", irq);
	set_val = mbox_reg->cpu_mbox_set[RECEIVE_CPU].cpu_mbox_int_int.mbox_int;
	int errno_mailbox[MAILBOX_MAX_NUM];
	cmdqu_t err_cmdq[MAILBOX_MAX_NUM];
	memset(errno_mailbox, 0, sizeof(errno_mailbox));
	memset(err_cmdq, 0, sizeof(err_cmdq));
	int erro_num = 0;
	for (i = 0; i < MAILBOX_MAX_NUM && set_val > 0; i++) 
	{
		/* valid_val uses unsigned char because of mailbox register table
		 * ~valid_val will be 0xFF
		 */
		unsigned char valid_val = set_val & (1 << i);

		pr_debug("MAILBOX_MAX_NUM = %d\n", MAILBOX_MAX_NUM);
		pr_debug("valid_val = %d set_val=%d i = %d\n", valid_val, set_val, i);
		if (valid_val) 
		{
			cmdqu_t linux_cmdq;
			
			cmdq = (cmdqu_t *)(mailbox_context) + i;
			/* mailbox buffer context is send from rtos, clear mailbox interrupt */
			mbox_reg->cpu_mbox_set[RECEIVE_CPU].cpu_mbox_int_clr.mbox_int_clr = valid_val;
			// need to disable enable bit
			mbox_reg->cpu_mbox_en[RECEIVE_CPU].mbox_info &= ~valid_val;
			// copy cmdq context (8 bytes) to buffer ASAP ??
			*((unsigned long long *) &linux_cmdq) = *((unsigned long long *)cmdq);
			/* need to clear mailbox interrupt before clear mailbox buffer ??*/
			*((unsigned long long *) cmdq) = 0;
			
			set_val &= (~(1<<i));

			if(linux_cmdq.resv.valid.rtos_valid == 0) 
			{
				pr_err("a mailbox contex 'linux2rtos' in linux irq_handler\n");
				continue;
			}
			/* mailbox buffer context is send from rtos */
			pr_debug("mailbox contex id : %d\n",i);
			pr_debug("cmdq=%p\n", cmdq);
			pr_debug("cmdq->ip_id =%d\n", linux_cmdq.ip_id);
			pr_debug("cmdq->cmd_id =%d\n", linux_cmdq.cmd_id);
			pr_debug("cmdq->param_ptr =%x\n", linux_cmdq.param_ptr);
			pr_debug("cmdq->block =%d\n", linux_cmdq.block);
			pr_debug("cmdq->linux_valid =%d\n", linux_cmdq.resv.valid.linux_valid);
			pr_debug("cmdq->rtos_valid =%x", linux_cmdq.resv.valid.rtos_valid);

			if(CMDID_INVALID(linux_cmdq.cmd_id)) 
			{
				pr_err("cmdid don't from rtos don't know!\n");
				continue;
			}

			if(g_cmd_handlers[linux_cmdq.cmd_id - CMDQU_SEND_TEST] == NULL) 
			{
				pr_err("linux cmdid don't register handler!\n");
				continue;
			}
			int ret = g_cmd_handlers[linux_cmdq.cmd_id - CMDQU_SEND_TEST](&linux_cmdq, g_private_data[linux_cmdq.cmd_id - CMDQU_SEND_TEST]);
			if(ret) 
			{
				errno_mailbox[i] = ret;
				err_cmdq[i] = *cmdq;
				erro_num++;
			}
		}
	}
	drv_spin_unlock_irqrestore(&mailbox_lock, flags);
	if(erro_num > 0) 
	{
		pr_err("linux irq_handler error! total amount: %d\n", erro_num);
		for (i = 0; i < MAILBOX_MAX_NUM; i++) 
		{
			if(errno_mailbox[i] != 0) 
			{
				pr_err("mailbox contex id %d errno %d\n", i, errno_mailbox[i]);
				pr_err("cmdq->ip_id =%d\n", err_cmdq[i].ip_id);
				pr_err("cmdq->cmd_id =%d\n", err_cmdq[i].cmd_id);
				pr_err("cmdq->param_ptr =%x\n", err_cmdq[i].param_ptr);
				pr_err("cmdq->block =%d\n", err_cmdq[i].block);
				pr_err("cmdq->linux_valid =%d\n", err_cmdq[i].resv.valid.linux_valid);
				pr_err("cmdq->rtos_valid =%x", err_cmdq[i].resv.valid.rtos_valid);
			}
		}
	}
	return IRQ_HANDLED;
}

static int sendtest_cmdqu_handler(cmdqu_t* cmdq, void* data)
{
	pr_debug("======send_test_recv=================\n");
	pr_debug("cmdq->ip_id =%d\n", cmdq->ip_id);
	pr_debug("cmdq->cmd_id =%d\n", cmdq->cmd_id);
	pr_debug("cmdq->param_ptr =%x\n", cmdq->param_ptr);
	pr_debug("cmdq->block =%d\n", cmdq->block);
	pr_debug("cmdq->linux_valid =%d\n", cmdq->resv.valid.linux_valid);
	pr_debug("cmdq->rtos_valid =%x", cmdq->resv.valid.rtos_valid);
	pr_debug("=====================================\n");
	return 0;
}

struct default_cmd_handler {
	const char* 			name;
	enum SYSTEM_CMD_TYPE 	cmd_id;
	cmdqu_irq_handler 		cmd_handler;
};

static struct default_cmd_handler dcmd_handler[] = 
{	
	{
		.name 	= 	"sendtest_cmd_handler",
		.cmd_id =	CMDQU_SEND_TEST, 
		.cmd_handler = sendtest_cmdqu_handler,
	},
};

long rtos_cmdqu_init(void)
{
	long ret = 0;
	int i;

	pr_debug("RTOS_CMDQU_INIT\n");
	spin_lock_init(&mailbox_queue_lock);
	mbox_reg = (struct mailbox_set_register *) reg_base;
	mbox_done_reg = (struct mailbox_done_register *) (reg_base + MAILBOX_DONE_OFFSET);
	mailbox_context = (unsigned long *) (reg_base + MAILBOX_CONTEXT_OFFSET);//MAILBOX_CONTEXT;

	pr_debug("mbox_reg=%p\n", mbox_reg);
	pr_debug("mbox_done_reg=%p\n", mbox_done_reg);
	pr_debug("mailbox_context=%p\n", mailbox_context);

	// init mailbox_context
	for ( i=0;i < MAILBOX_MAX_NUM;i++)
		mailbox_context[i] = 0;
	/* init sqirq parameters*/

	for ( i=0;i < MAX_CMD_NUM;i++) 
	{
		g_cmd_handlers[i] = NULL;
		g_private_data[i] = NULL;
	}
	for ( i=0;i < sizeof(dcmd_handler)/sizeof(dcmd_handler[0]); i++)
	{
		struct default_cmd_handler* cmd = &(dcmd_handler[i]);
		int r = request_cmdqu_irq(cmd->cmd_id, cmd->cmd_handler, NULL);
		BUG_ON(r!=0);
	}

	return ret;
}

long rtos_cmdqu_deinit(void)
{
	long ret = 0;
	pr_debug("RTOS_CMDQU_DEINIT\n");
	//mailbox deinit
	return ret;
}

int rtos_cmdqu_send(cmdqu_t *cmdq)
{
	int ret = 0;
	int valid;
	unsigned long flags;
	int mb_flags;
	cmdqu_t *linux_cmdqu_t;

	pr_debug("RTOS_CMDQU_SEND\n");
	
	spin_lock_irqsave(&mailbox_queue_lock, flags);
	// when linux and rtos send command at the same time, it might cause a problem.
	// might need to spinlock with rtos, do it later
	drv_spin_lock_irqsave(&mailbox_lock, mb_flags);
	if (mb_flags == MAILBOX_LOCK_FAILED) 
	{
		pr_err("ip_id=%d cmd_id=%d param_ptr=%x\n", cmdq->ip_id, cmdq->cmd_id, (unsigned int)cmdq->param_ptr);
		spin_unlock_irqrestore(&mailbox_queue_lock, flags);
		return -EBUSY;
	}
	linux_cmdqu_t = (cmdqu_t *) mailbox_context;
	for (valid = 0; valid < MAILBOX_MAX_NUM; valid++) 
	{
		if (linux_cmdqu_t->resv.valid.linux_valid == 0 && linux_cmdqu_t->resv.valid.rtos_valid == 0) 
		{
			// mailbox buffer context is int (4 bytes) access
			int *ptr = (int *)linux_cmdqu_t;
			linux_cmdqu_t->resv.valid.linux_valid = 1;
			*ptr = ((cmdq->ip_id << 0) | (cmdq->cmd_id << 8) | (cmdq->block << 15) |
					(linux_cmdqu_t->resv.valid.linux_valid << 16) |
					(linux_cmdqu_t->resv.valid.rtos_valid << 24));
			linux_cmdqu_t->param_ptr = cmdq->param_ptr;
			pr_debug("mailbox contexid = %d\n", valid);
			pr_debug("linux_valid = %d\n", linux_cmdqu_t->resv.valid.linux_valid);
			pr_debug("rtos_valid = %d\n", linux_cmdqu_t->resv.valid.rtos_valid);
			pr_debug("ip_id = %d\n", linux_cmdqu_t->ip_id);
			pr_debug("cmd_id = %d\n", linux_cmdqu_t->cmd_id);
			pr_debug("block = %d\n", linux_cmdqu_t->block);
			pr_debug("param_ptr = %x\n", linux_cmdqu_t->param_ptr);
			pr_debug("*ptr = %x\n", *ptr);
			// clear mailbox
			mbox_reg->cpu_mbox_set[SEND_TO_CPU].cpu_mbox_int_clr.mbox_int_clr = (1 << valid);
			// trigger mailbox valid to rtos
			mbox_reg->cpu_mbox_en[SEND_TO_CPU].mbox_info |= (1 << valid);
			mbox_reg->mbox_set.mbox_set = (1 << valid);
			break;
		}
		linux_cmdqu_t++;
	}

	if (valid >= MAILBOX_MAX_NUM) 
	{
		pr_err("No valid mailbox is available\n");
		drv_spin_unlock_irqrestore(&mailbox_lock, mb_flags);
		spin_unlock_irqrestore(&mailbox_queue_lock, flags);
		return -ENOBUFS;
	}
	drv_spin_unlock_irqrestore(&mailbox_lock, mb_flags);
	spin_unlock_irqrestore(&mailbox_queue_lock, flags);
    return ret;
}
EXPORT_SYMBOL(rtos_cmdqu_send);

static int cvi_rtos_cmdqu_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cvi_rtos_cmdqu_device *ndev;
	struct resource *res;
	int err = -1;

	pr_info("%s start ---\n", __func__);
	pr_info("name=%s\n", pdev->name);
	ndev = devm_kzalloc(&pdev->dev, sizeof(*ndev), GFP_KERNEL);
	if (!ndev)
		return -ENOMEM;

	ndev->dev = dev;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	reg_base = (__u64)devm_ioremap(&pdev->dev, res->start, res->end - res->start);

	pr_info("res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%llx).\n", res->start, res->end, le64_to_cpu(reg_base));

	// spinlock ip offset address (0xc0)
	spinlock_base(reg_base + 0xc0);
	mailbox_irq = platform_get_irq_byname(pdev, "mailbox");

	/* init cmdqu*/
	rtos_cmdqu_init();
	platform_set_drvdata(pdev, ndev);

	err = request_irq(mailbox_irq, rtos_irq_handler, 0, "mailbox", (void *)ndev);

	if (err) {
		pr_err("fail to register interrupt handler\n");
		return -1;
	}

	pr_info("%s DONE\n", __func__);
	return 0;
}

static int cvi_rtos_cmdqu_remove(struct platform_device *pdev)
{
	struct cvi_rtos_cmdqu_device *ndev = platform_get_drvdata(pdev);

	platform_set_drvdata(pdev, NULL);
	/* remove irq handler*/
	free_irq(mailbox_irq, ndev);
	rtos_cmdqu_deinit();
	pr_debug("%s DONE\n", __func__);
	return 0;
}

static const struct of_device_id cvi_rtos_cmdqu_match[] = {
	{ .compatible = "cvitek,rtos_cmdqu" },
	{},
};

static struct platform_driver cvi_rtos_cmdqu_driver = {
	.probe = cvi_rtos_cmdqu_probe,
	.remove = cvi_rtos_cmdqu_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "rtos_cmdqu",
		.of_match_table = cvi_rtos_cmdqu_match,
	},
};


static __exit void cvi_rtos_cmdqu_exit(void)
{
	platform_driver_unregister(&cvi_rtos_cmdqu_driver);
	cvi_spinlock_uninit();
	pr_debug("%s DONE\n", __func__);
}

static __init int cvi_rtos_cmdqu_init(void)
{
	pr_info("cvi_rtos_cmdqu_init\n");
	platform_driver_register(&cvi_rtos_cmdqu_driver);
	pr_debug("%s done\n", __func__);
	cvi_spinlock_init();
	cmdqu_t test_cmd;
	test_cmd.cmd_id = CMDQU_SEND_TEST;
	test_cmd.ip_id = 0;
	test_cmd.block = 0;
	test_cmd.param_ptr = 0;
	test_cmd.resv.mstime = 0;
	BUG_ON(rtos_cmdqu_send(&test_cmd) != 0);
	mdelay(1000);
	test_cmd.cmd_id = CMDQU_DUO_TEST;
	test_cmd.ip_id = 0x234;
	test_cmd.block = 1;
	test_cmd.param_ptr = 3;
	test_cmd.resv.mstime = 4;
	BUG_ON(rtos_cmdqu_send(&test_cmd) != 0);
	mdelay(1000);

	test_cmd.cmd_id = 123;
	test_cmd.ip_id = 0x234;
	test_cmd.block = 1;
	test_cmd.param_ptr = 3;
	test_cmd.resv.mstime = 4;
	BUG_ON(rtos_cmdqu_send(&test_cmd) != 0);
	mdelay(1000);

	test_cmd.cmd_id = CMDQU_SEND_TEST;
	test_cmd.ip_id = 0;
	test_cmd.block = 0;
	test_cmd.param_ptr = 0;
	test_cmd.resv.mstime = 0;
	int i;
	for(i =0;i<10;i++)
	{
		rtos_cmdqu_send(&test_cmd);
	}
	mdelay(1000);

	return 0;
}

MODULE_DESCRIPTION("RTOS_CMD_QUEUE");
MODULE_LICENSE("GPL");
module_init(cvi_rtos_cmdqu_init);
module_exit(cvi_rtos_cmdqu_exit);
