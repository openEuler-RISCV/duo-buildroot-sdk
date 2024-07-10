/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023. All rights reserved
 *
 * SPDX-License-Identifier: GPL-2.0
 */

/* 
 *  milkv-duo specific
 *  on milkv-duo, three core, C906L, C906B, 8051
 *  only C906B is running Linux, and C906L, 8051 is not managed by Linux
 *
 * /

/*
 * DESCRPTION:
 *      this is the kernel module that support device file "/dev/mcs" for openEuler/mcs
 *      also called "openEuler/mcs kernel dependency"
 *      "openEuler/mcs User Program" will interact with this kernel module by device file "/dev/mcs"
 * NOTE:
 *      this is specific kernel module for milkv-duo, and it will use mcs_rtoscmdqu api [this is supported in openEuler-RISC-V/duo-buildroot]
 *      "mcs_rtoscmdqu" will support for mailbox send and request mailbox interrupt
 */

#define pr_fmt(fmt) "mcs: " fmt

#include <linux/device.h>
#include <linux/init.h>
#include <linux/file.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/cpumask.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/version.h>
#include <linux/random.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/io.h>
#include "mcs_cmdqu/rtos_cmdqu.h"

#define MCS_DEVICE_NAME		"mcs"

/**
 * PSCI Functions
 * For more detail see:
 * Arm Power State Coordination Interface Platform Design Document
 * It is also magic number for the flag of rtos state
 * it will write in rsc_table "reserved" field to represent the state of rtos
 */
#define CPU_ON_FUNCID		    0xC4000003
#define AFFINITY_INFO_FUNCID	0xC4000004

#define MAGIC_NUMBER		'A'
#define IOC_SENDIPI		    _IOW(MAGIC_NUMBER, 0, int)
#define IOC_CPUON		    _IOW(MAGIC_NUMBER, 1, int)
#define IOC_AFFINITY_INFO	_IOW(MAGIC_NUMBER, 2, int)
#define IOC_QUERY_MEM		_IOW(MAGIC_NUMBER, 3, int)
#define IOC_MAXNR		    3
#define RPROC_MEM_MAX		4

static struct class *mcs_class;
static int mcs_major;

/*
 *  for milkv-duo now only support 
 *  CPU 2 -> C906L[Run UniProton]
 *  C906B 1
 *  C906L 2
 */
struct cpu_info {
	u32 cpu;
	u64 boot_addr;
};

/**
 * struct mcs_rproc_mem - internal memory structure
 * @phy_addr: physical address of the memory region
 * @size: total size of the memory region
 */
struct mcs_rproc_mem {
	u64 phy_addr;
	u64 size;
};
static struct mcs_rproc_mem mem[RPROC_MEM_MAX];

static DECLARE_WAIT_QUEUE_HEAD(mcs_wait_queue);
static atomic_t irq_ack;


static int mcs_rtos_callback(cmdqu_t* cmdq, void* data)
{
    BUG_ON(cmdq == NULL);
    BUG_ON(data != NULL);
    BUG_ON(cmdq->cmd_id != CMDQU_MCS_COMMUNICATE);
    pr_info("received ipi from client os\n");
	atomic_set(&irq_ack, 1);
	wake_up_interruptible(&mcs_wait_queue);
    return 0;
}

static int init_mcs_ipi(void)
{
	int err;
    err = request_cmdqu_irq(CMDQU_MCS_COMMUNICATE, mcs_rtos_callback, NULL);
	return err;
}

static void remove_mcs_ipi(void)
{
	request_cmdqu_irq(CMDQU_MCS_COMMUNICATE, NULL, NULL);
	return;

}

static int __private_boot_cpu(struct cpu_info* cpu)
{
    uint8_t pakcage_id;
    get_random_bytes(&pakcage_id, 1);
    int ret;
    cmdqu_t cmd_boot;
    cmd_boot.ip_id = pakcage_id;
    cmd_boot.cmd_id = CMDQU_MCS_BOOT;
    cmd_boot.block = 0;
    cmd_boot.resv.mstime = 0;
    cmd_boot.param_ptr = (uint32_t)((cpu->boot_addr) & 0xffffffff);
    ret = rtos_cmdqu_send(&cmd_boot);
    if(ret)
        return ret;
    cmd_boot.ip_id = pakcage_id + 1;
    cmd_boot.cmd_id = CMDQU_MCS_BOOT;
    cmd_boot.block = 0;
    
    cmd_boot.resv.mstime = 0;
    cmd_boot.param_ptr = (uint32_t)(((cpu->boot_addr) & 0xffffffff00000000) >> 32);
    ret = rtos_cmdqu_send(&cmd_boot);
    if(ret)
        return ret;
    return 0;
}


static int send_clientos_ipi(u32 cpuid)
{
    int ret;
    //only support for CPU2 C906L
    if(cpuid == 2)
    {
        cmdqu_t cmd_communicate;
        cmd_communicate.ip_id = 0;
        cmd_communicate.cmd_id = CMDQU_MCS_COMMUNICATE;
        cmd_communicate.block = 0;
        cmd_communicate.resv.mstime = 0;
        cmd_communicate.param_ptr = 0;
        ret = rtos_cmdqu_send(&cmd_communicate);
    }
    else
    {
        ret = -EINVAL;
        pr_err("unkown cpu id for /dev/mcs %d in send_clientos_ipi\n", cpuid);
    }
    return ret;
}

static unsigned int mcs_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;

	poll_wait(file, &mcs_wait_queue, wait);
	if (atomic_cmpxchg(&irq_ack, 1, 0) == 1)
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

static long mcs_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	int ret;
	struct cpu_info info;

	if (_IOC_TYPE(cmd) != MAGIC_NUMBER)
		return -EINVAL;
	if (_IOC_NR(cmd) > IOC_MAXNR)
		return -EINVAL;
	if (cmd != IOC_QUERY_MEM) 
    {
		ret = copy_from_user(&info, (struct cpu_info __user *)arg, sizeof(info));
		if (ret)
			return -EFAULT;
	}

	switch (cmd) 
    {
	    case IOC_SENDIPI:
            pr_info("received ioctl cmd to send ipi to cpu(%d)\n", info.cpu);
            ret = send_clientos_ipi(info.cpu);
            if(ret)
                return ret;
            break;

	    case IOC_CPUON:
            if(info.cpu != 2) 
            {
                pr_err("error cpuid, milkv-duo only support boot C906L, CPU2!\n");
                return -EINVAL;
            }
            pr_info("start booting clientos on cpu%d(%llx)\n", info.cpu, info.boot_addr);
            ret = __private_boot_cpu(&info);
            if(ret)
                return ret;
            break;

        case IOC_AFFINITY_INFO:
            return -EFAULT;
            break;

        case IOC_QUERY_MEM:
            if (copy_to_user((void __user *)arg, &mem[0], sizeof(mem[0])))
                return -EFAULT;
            break;

        default:
            pr_err("IOC param invalid(0x%x)\n", cmd);
            return -EINVAL;
            break;
	}
	return 0;
}

static pgprot_t mcs_phys_mem_access_prot(struct file *file, unsigned long pfn,
                                         unsigned long size, pgprot_t vma_prot)
{
        return pgprot_noncached(vma_prot);
}

static const struct vm_operations_struct mmap_mem_ops = {
#ifdef CONFIG_HAVE_IOREMAP_PROT
	.access = generic_access_phys
#endif
};

/* A lite version of linux/drivers/char/mem.c, Test with MMU for arm64 mcs functions */
static int mcs_mmap(struct file *file, struct vm_area_struct *vma)
{
    int i;
    int found = 0;
    size_t size = vma->vm_end - vma->vm_start;
    
    
    phys_addr_t offset = (phys_addr_t)vma->vm_pgoff << PAGE_SHIFT;
    /* Does it even fit in phys_addr_t? */
    if (offset >> PAGE_SHIFT != vma->vm_pgoff)
            return -EINVAL;
    /* It's illegal to wrap around the end of the physical address space. */
    if (offset + (phys_addr_t)size - 1 < offset)
            return -EINVAL;
    for (i = 0; (i < RPROC_MEM_MAX) && (mem[i].phy_addr != 0 ); i++) 
    {
        if (offset >= mem[i].phy_addr && (offset + size -1) <= (mem[i].phy_addr + mem[i].size -1)) 
        {
            found = 1;
            break;
        }
    }

    if (found == 0) 
    {
        pr_err("mmap failed: mmap memory is not in mcs reserved memory %llx\n", offset);
        return -EINVAL;
    }
    
    pr_info("mcs_mmap: want /dev/mcs address: %llx want size %llx\n", offset, size);
    
    vma->vm_page_prot = mcs_phys_mem_access_prot(file, vma->vm_pgoff,
                                                 size,
                                                 vma->vm_page_prot);

    vma->vm_ops = &mmap_mem_ops;

    /* Remap-pfn-range will mark the range VM_IO */
    if (remap_pfn_range(vma,
                        vma->vm_start,
                        vma->vm_pgoff,
                        size,
                        vma->vm_page_prot) < 0)
            return -EAGAIN;
    return 0;
}

static int mcs_open(struct inode *inode, struct file *filp)
{
    if (!capable(CAP_SYS_RAWIO))
		return -EPERM;
	return 0;
}

static const struct file_operations mcs_fops = {
	.open = mcs_open,
	.mmap = mcs_mmap,
	.poll = mcs_poll,
	.unlocked_ioctl = mcs_ioctl,
	.compat_ioctl = compat_ptr_ioctl,
	.llseek = generic_file_llseek,
};

static int init_reserved_mem(void)
{
    int n = 0;
    int i, count, ret;
    struct device_node *np;
    np = of_find_compatible_node(NULL, NULL, "oe,mcs_remoteproc");
    if (np == NULL)
            return -ENODEV;

    count = of_count_phandle_with_args(np, "memory-region", NULL);
    if (count <= 0) 
    {
            pr_err("reserved mem is required for MCS\n");
            return -ENODEV;
    }

    for (i = 0; i < count; i++) 
    {
            struct device_node *node;
            struct resource res;

            node = of_parse_phandle(np, "memory-region", i);
            ret = of_address_to_resource(node, 0, &res);
            if (ret) 
            {
                    pr_err("unable to resolve memory region\n");
                    return ret;
            }

            if (n >= RPROC_MEM_MAX)
                    break;

            if (!request_mem_region(res.start, resource_size(&res), "mcs_mem")) 
            {
                    pr_err("Can not request mcs_mem, 0x%llx-0x%llx\n", res.start, res.end);
                    return -EINVAL;
            }
            mem[n].phy_addr = res.start;
            mem[n].size = resource_size(&res);
            n++;
    }

    for (i = 0; (i < RPROC_MEM_MAX) && (mem[i].phy_addr != 0); i++)
            pr_info("memory for uniproton : id %d phy_addr %llx size %llx \n", i, mem[i].phy_addr, mem[i].size);
    return 0;
}

static void release_reserved_mem(void)
{
	int i;
	for (i = 0; (i < RPROC_MEM_MAX) && (mem[i].phy_addr != 0); i++) 
    {
		release_mem_region(mem[i].phy_addr, mem[i].size);
	}
}

static int register_mcs_dev(void)
{
	int ret;
	struct device *mcs_dev;

	mcs_major = register_chrdev(0, MCS_DEVICE_NAME, &mcs_fops);
	if (mcs_major < 0) 
    {
		ret = mcs_major;
		pr_err("register_chrdev failed (%d)\n", ret);
		goto err;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	mcs_class = class_create(MCS_DEVICE_NAME);
#else
	mcs_class = class_create(THIS_MODULE, MCS_DEVICE_NAME);
#endif
	if (IS_ERR(mcs_class)) 
    {
		ret = PTR_ERR(mcs_class);
		pr_err("class_create failed (%d)\n", ret);
		goto err_class;
	}

	mcs_dev = device_create(mcs_class, NULL, MKDEV(mcs_major, 0), NULL, MCS_DEVICE_NAME);
	if (IS_ERR(mcs_dev)) 
    {
		ret = PTR_ERR(mcs_dev);
		pr_err("device_create failed (%d)\n", ret);
		goto err_device;
	}
	return 0;

err_device:
	class_destroy(mcs_class);
err_class:
	unregister_chrdev(mcs_major, MCS_DEVICE_NAME);
err:
	return ret;
}

static void unregister_mcs_dev(void)
{
	device_destroy(mcs_class, MKDEV(mcs_major, 0));
	class_destroy(mcs_class);
	unregister_chrdev(mcs_major, MCS_DEVICE_NAME);
}

static int __init mcs_dev_init(void)
{
	int ret;

	ret = init_reserved_mem();
	if (ret) 
    {
		pr_err("Failed to get mcs mem, ret = %d\n", ret);
		return ret;
	}

	ret = init_mcs_ipi();
	if (ret) 
    {
		pr_err("Failed to init mcs ipi, ret = %d\n", ret);
		goto err_free_mcs_mem;
	}

	ret = register_mcs_dev();
	if (ret) 
    {
		pr_err("Failed to register mcs dev, ret = %d\n", ret);
		goto err_remove_ipi;
	}

    pr_info("inited mcs dev!\n");
	return ret;
err_remove_ipi:
	remove_mcs_ipi();
err_free_mcs_mem:
	release_reserved_mem();
	return ret;
}
module_init(mcs_dev_init);


static void __exit mcs_dev_exit(void)
{
	remove_mcs_ipi();
	unregister_mcs_dev();
	release_reserved_mem();
	pr_info("remove mcs dev\n");
}
module_exit(mcs_dev_exit);


MODULE_AUTHOR("openEuler Embedded");
MODULE_DESCRIPTION("mcs device");
MODULE_LICENSE("Dual BSD/GPL");
