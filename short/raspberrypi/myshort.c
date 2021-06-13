#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/slab.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("jackx");
MODULE_DESCRIPTION("A simple short module");

volatile unsigned int* GPFSEL0 = NULL;
volatile unsigned int* GPSET0 = NULL;
volatile unsigned int* GPCLR0 = NULL;

struct cdev my_cdev;

static int myshort_open(struct inode *inodp, struct file *filp)
{
    printk(KERN_INFO "myshort_open\n");

    //*GPFSEL0 &= ~(0x6 << 12);
    //*GPFSEL0 |= (0x1 << 12);
    unsigned int orig_value = ioread32(GPFSEL0);
    iowrite32(orig_value & ~ (0x6 << 12) | (0x1 << 12), GPFSEL0);

    return 0;
}

static ssize_t myshort_read(struct file *filp, char __user *pbuf, size_t count, loff_t *f_pos)
{
    printk(KERN_INFO "myshort_read: count=%d\n", count);
    return count;
}

static ssize_t myshort_write(struct file *filp, const char __user *pbuf, size_t count, loff_t *f_pos)
{
    unsigned char* kbuf = kmalloc(count, GFP_KERNEL);
    printk(KERN_INFO "write  kbuf is %p\n", kbuf);

    if (count > 0)
    {
        if (copy_from_user(kbuf, pbuf, count) != 0)
        {
            printk(KERN_ALERT "myshort_write: copy error!\n");
            return -EFAULT;
        }
	printk(KERN_INFO "user input is %s\n", kbuf);
	if (kbuf[0] == '1')
	{
	    printk (KERN_INFO "write 1\n");
	    //*GPSET0 |= 0x1 << 4;
	    iowrite32(ioread32(GPSET0) | (0x1 << 4), GPSET0);
	}
	else if (kbuf[0] == '0')
	{
            printk (KERN_INFO "write 0\n");
	    //*GPCLR0 |= 0x1 << 4;
	    iowrite32(ioread32(GPCLR0) | (0x1 << 4), GPCLR0);
	}
	else
	{
            printk (KERN_INFO "doing nothing\n");
	}
    }
    kfree(kbuf);
    return count;
}

struct file_operations myshort_ops = {
    .owner = THIS_MODULE,
    .read = myshort_read,
    .write = myshort_write,
    .open = myshort_open,
};

dev_t myshort_dev_no;

static int myshort_init(void)
{
    int ret = 0;

    ret = alloc_chrdev_region(&myshort_dev_no, 0, 1, "myshort"); 
    if (ret < 0)
    {
        printk(KERN_WARNING "myshort: can't get major dev.");
        return ret;
    }

    cdev_init(&my_cdev, &myshort_ops);
    my_cdev.owner = THIS_MODULE;
    ret = cdev_add(&my_cdev, myshort_dev_no, 1);
    if (ret)
    {
        printk(KERN_ALERT "can't add cdev\n");
        return ret;
    }

    GPFSEL0 = (volatile unsigned int *)ioremap(0x3f200000, 4);
    printk(KERN_INFO "GPFSEL0 is %p\n", GPFSEL0);
    GPSET0 = (volatile unsigned int *)ioremap(0x3f20001c, 4);
    printk(KERN_INFO "GPSET0 is %p\n", GPSET0);
    GPCLR0 = (volatile unsigned int *)ioremap(0x3f200028, 4);
    printk(KERN_INFO "GPCLR0 is %p\n", GPCLR0);

    printk(KERN_ALERT "Hello world: this is my short!\n");
    return 0;
}

static void myshort_exit(void)
{
    unregister_chrdev_region(myshort_dev_no, 1);
    
    iounmap(GPFSEL0);
    iounmap(GPSET0);
    iounmap(GPCLR0);

    printk(KERN_ALERT "Goodbye, cruel world!\n");
}

module_init(myshort_init);
module_exit(myshort_exit);
