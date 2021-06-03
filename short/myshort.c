#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/slab.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("jackx");
MODULE_DESCRIPTION("A simple short module");


#define SHORT_NR_PORTS	8	/* use 8 ports by default */

static unsigned long base = 0x378;
//static unsigned long base = 0x60;

struct cdev my_cdev;

static int myshort_open(struct inode *inodp, struct file *filp)
{
    printk(KERN_INFO "myshort_open\n");
    return 0;
}

static ssize_t myshort_read(struct file *filp, char __user *pbuf, size_t count, loff_t *f_pos)
{
    unsigned long port = base; 
    unsigned char *kbuf = kmalloc(count, GFP_KERNEL);
    unsigned char *p = kbuf; 
    size_t length = count;
    void *address = (void *)base;

    //ioport_map(address, 0);

    if (count > 0)
    {
        while (length--)
        {
           *(p++) = inb(port);
           //*(p++) = ioread8(address);
        }

        if (copy_to_user(pbuf, kbuf, count) != 0)
        {
            printk(KERN_ALERT "myshort_read: copy error!\n");
            return 0;
        }
    }
    //ioport_unmap(address);
    printk(KERN_INFO "myshort_read: count=%d, content=%s\n", count, kbuf);
    kfree(kbuf);
    return count;
}

static ssize_t myshort_write(struct file *filp, const char __user *pbuf, size_t count, loff_t *f_pos)
{
    unsigned long port = base; 
    unsigned char *kbuf = kmalloc(count, GFP_KERNEL);
    unsigned char *p = kbuf; 
    size_t length = count;

    if (count > 0)
    {
        if (copy_from_user(kbuf, pbuf, count) != 0)
        {
            printk(KERN_ALERT "myshort_write: copy error!\n");
            return -EFAULT;
        }

        while (length--)
        {
            outb_p(*(p++), port);
        }
    }
    printk(KERN_INFO "myshort_write: count=%d, content=%s\n", count, kbuf);
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

    if (!request_region(base, SHORT_NR_PORTS, "myshort"))
    {
        printk(KERN_ALERT "can't get I/O port 0x%lx!\n", base);
        return -ENODEV;
    }

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

    printk(KERN_ALERT "Hello world: this is my short!\n");
    return 0;
}

static void myshort_exit(void)
{
    unregister_chrdev_region(myshort_dev_no, 1);
    release_region(base,SHORT_NR_PORTS);
    printk(KERN_ALERT "Goodbye, cruel world!\n");
}

module_init(myshort_init);
module_exit(myshort_exit);
