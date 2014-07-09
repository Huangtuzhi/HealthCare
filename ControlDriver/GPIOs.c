#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <asm/irq.h>
#include <mach/regs-gpio.h>
#include <mach/hardware.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/pci.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/unistd.h>
#include <linux/slab.h>

#define DEVICE_NAME "GPIOs"

static unsigned long gpio_table[4]={
	S3C6410_GPF(0),//映射的物理地址，选取两个GPIO口控制气泵和电磁阀
	S3C6410_GPF(1),
	S3C6410_GPF(2),
	S3C6410_GPF(3),
};

static unsigned int gpio_cfg_table[4]={
	S3C2410_GPIO_OUTPUT,
	S3C2410_GPIO_OUTPUT,
	S3C2410_GPIO_OUTPUT,
	S3C2410_GPIO_OUTPUT,
};

static int gpio_ioctl(struct inode *inode,struct file *file,unsigned int cmd,unsigned long arg)
{
	switch(cmd){
	case 0:
	case 1:
		if(arg>4){
			return -EINVAL;
		}
		s3c2410_gpio_setpin(gpio_table[arg],!cmd);
		return 0;
	default:
		return -EINVAL;
	}
}

static struct file_operations dev_fops={
	.owner=THIS_MODULE,
    .ioctl=gpio_ioctl,
};

static struct miscdevice misc={
	.minor=MISC_DYNAMIC_MINOR,//自动分配子设备号
	.name=DEVICE_NAME,
	.fops=&dev_fops,
};

static int __init dev_init(void)
{
	int ret;
    int i;
	for(i=0;i<4;i++){
		s3c2410_gpio_cfgpin(gpio_table[i],gpio_cfg_table[i]);//设置GPIO引脚类型
	    s3c2410_gpio_setpin(gpio_table[i],0);
	}
	ret=misc_register(&misc);
	printk(DEVICE_NAME "\t initialization is completed\n");
	return ret;
}

static void __exit dev_exit(void)
{
	misc_deregister(&misc);
}

module_init(dev_init);
module_exit(dev_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huang Yi");

	


