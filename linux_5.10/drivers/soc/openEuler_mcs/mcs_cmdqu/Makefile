obj-$(CONFIG_MCS_MAILBOX) := cvi_mbox.o
cvi_mbox-y := rtos_cmdqu.o \
			cvi_spinlock.o

ccflags-y += -I$(srctree)/$(src)/
ccflags-$(CONFIG_DYNAMIC_DEBUG) += -DDEBUG
