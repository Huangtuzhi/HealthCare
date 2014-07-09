#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/serio.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <mach/regs-clock.h>
#include <plat/regs-timer.h>
	 
#include <plat/regs-adc.h> //定义在\arch\arm\plat-s3c\include\plat
#include <mach/regs-gpio.h> //定义在arch/arm/mach-s3c2410/include/mach
#include <linux/cdev.h>
#include <linux/miscdevice.h>

//#include "s3c24xx-adc.h"
#undef DEBUG
//#define DEBUG
#ifdef DEBUG
#define DPRINTK(x...) {printk(__FUNCTION__"(%d): ",__LINE__);printk(##x);}
#else
#define DPRINTK(x...) (void)(0)
#endif

#define DEVICE_NAME	"myadc"

static void __iomem *base_addr;

typedef struct{
	wait_queue_head_t wait;//等待队列
	int channel;//通道
	int prescale;//转换速率
}ADC_DEV;

DECLARE_MUTEX(ADC_LOCK);
static int OwnADC=0;

static ADC_DEV adcdev;
static volatile int ev_adc=0;
static int adc_data;

static struct clk *adc_clock;

#define ADCCON  (*(volatile unsigned long*)(base_addr+S3C2410_ADCCON))
#define ADCTSC  (*(volatile unsigned long*)(base_addr+S3C2410_ADCTSC))
#define ADCDLY  (*(volatile unsigned long*)(base_addr+S3C2410_ADCDLY))
#define ADCDAT0 (*(volatile unsigned long*)(base_addr+S3C2410_ADCDAT0))
#define ADCDAT1 (*(volatile unsigned long*)(base_addr+S3C2410_ADCDAT1))
#define ADCUPDN (*(volatile unsigned long*)(base_addr+0x14))

#define PRESCALE_DIS (0 << 14)
#define PRESCALE_EN  (1 << 14)
#define PRESCVL(x)  ((x)<<6)
#define ADC_INPUT(x) ((x)<<3) //模拟输入通道
#define ADC_START    (1 << 0)
#define ADC_ENDCVT   (1 <<15)

//设置ADC转换参数
#define START_ADC_AIN(ch,prescale) \
	do{\
		ADCCON = PRESCALE_EN | PRESCVL(prescale) | ADC_INPUT((ch)); \
		ADCCON |=ADC_START; \
	}while (0)

//注册ADC中断服务子函数
static irqreturn_t adcdone_int_handler(int irq, void *dev_id)
{
	if(OwnADC){
		adc_data = ADCDAT0 &  0x3ff;
		ev_adc = 1;
		wake_up_interruptible(&adcdev.wait);
	}

	return IRQ_HANDLED;
}

static ssize_t adc_read(struct file *filp, char *buffer, size_t count, loff_t *ppos)
{
	char str[20];
	int value;
	size_t len;
	if (down_trylock(&ADC_LOCK) ==0){
		OwnADC=1;
		START_ADC_AIN(adcdev.channel, adcdev.prescale);
		wait_event_interruptible(adcdev.wait, ev_adc);
		ev_adc=0;
		DPRINTK("AIN[%d]=0x%04x, %d\n",adcdev.channel, adc_data, ADCCON & 0x80?1:0);//&0x80是神马作用
        
		value=adc_data;

		OwnADC=0;
		up(&ADC_LOCK);
	}
	else{
		value=-1;
	}

	len=sprintf(str, "%d\n",value);
	if(count >=len){
		int r = copy_to_user(buffer, str, len);
		return r ? r :len;
	}else{
		return -EINVAL;
	}

}

static int adc_open(struct inode *inode, struct file *filp)
{
	init_waitqueue_head(&(adcdev.wait));
	adcdev.channel=1;//采用AIN1 位于GPIO座上6
	adcdev.prescale=0xf9;//预分频系数为249 分频率后的采样率为50M/250/5cycles=40K

	DPRINTK("ADC opened\n");
	return 0;
}

static int adc_release(struct inode *inode, struct file *filp)
{
	DPRINTK("adc closed \n");
	return 0;
}

static struct file_operations dev_fops={
	.owner=THIS_MODULE,
	.open=adc_open,
	.read=adc_read,
	.release=adc_release,
};

static struct miscdevice misc={
	.minor=MISC_DYNAMIC_MINOR,
	.name=DEVICE_NAME,
	.fops=&dev_fops,
};

static int __init dev_init(void)
{
		int ret;

		base_addr=ioremap(S3C2410_PA_ADC,0x20);
		if(base_addr == NULL){
			printk(KERN_ERR "Failed to remap register block \n");
			return -ENOMEM;
		}

		adc_clock=clk_get(NULL,"adc");
		if(!adc_clock){
			printk(KERN_ERR "failed to get adc clock source \n");
			return -ENOENT;
		}
		clk_enable(adc_clock);//使能ADC系统时钟

		//普通ADC采样功能
		ADCTSC =0;

		ret = request_irq(IRQ_ADC, adcdone_int_handler, IRQF_SHARED, DEVICE_NAME, &adcdev);
		if(ret){
			iounmap(base_addr);//什么时候取消映射 if -1也为真
			return ret;
		}

		ret=misc_register(&misc);
		printk(DEVICE_NAME "\t initialized\n");
		return ret;
}

static void __exit dev_exit(void)
{
	free_irq(IRQ_ADC, &adcdev);
	iounmap(base_addr);

	if(adc_clock){
		clk_disable(adc_clock);
		clk_put(adc_clock);
		adc_clock=NULL;
	}

	misc_deregister(&misc);
}

ECPORT_SYMBOL(ADC_LOCK);
module_init(dev_init);
module_exit(dev_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huang Yi");





