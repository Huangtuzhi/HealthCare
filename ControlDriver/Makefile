obj-m:=GPIOs.o
CURRENT_PATH:=$(shell pwd)
ARM_LINUX_KERNEL:=/opt/linux-2.6.38
all:
	$(MAKE) -C $(ARM_LINUX_KERNEL) SUBDIRS=$(CURRENT_PATH)
modulesclean:
	rm -rf *.cmd *.o *.ko *.mod *.symvers *.order
