
# mcs_cmdqu and mcs all are init default compiled in kernel
# use module_init -> same priority , actually we need init mcs_cmdqu first
# for the same init priority , the actually priority is defined by who compile first
# use  "obj-y += mcs_cmdqu/" before "obj-y += mcs.o" let mcs_cmdqu.c compile first
# it will firt init mcs_cmdqu , then init mcs
obj-y		  += mcs_cmdqu/

obj-$(CONFIG_MCS) += mcs.o
