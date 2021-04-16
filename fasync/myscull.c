#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include "myscull.h"
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/fs.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("jackx");
MODULE_DESCRIPTION("A simple scull module");

struct myscull_dev
{
    struct cdev my_cdev;
    char* buf;
    int size;
    int capacity;
    wait_queue_head_t qread;
    wait_queue_head_t qwrite;
    struct fasync_struct *qsync;
} chardev1;

static int myscull_open(struct inode *inodp, struct file *filp)
{
    struct cdev* pcdev = inodp->i_cdev;
    struct myscull_dev* pmydev = container_of(pcdev, struct myscull_dev, my_cdev);
    filp->private_data = pmydev;
    printk(KERN_INFO "myscull_open: capacity=%d, size=%d, f_pos=%ld\n", pmydev->capacity, pmydev->size, filp->f_pos);
    return 0;
}

static ssize_t myscull_read(struct file *filp, char __user *pbuf, size_t count, loff_t *f_pos)
{
    struct myscull_dev* pmydev = (struct myscull_dev*)(filp->private_data);
    printk(KERN_INFO "myscull_read: enter count is %ld, size = %d, f_pos is %ld, filp->f_pos is %ld\n", count, pmydev->size, *f_pos, filp->f_pos);
    count = (*f_pos + count < pmydev->size) ? count : (pmydev->size - *f_pos);
    if (count > 0)
    {
        while (pmydev->size == 0)
        {
            if (wait_event_interruptible(pmydev->qread, pmydev->size != 0))
            {
                printk(KERN_INFO "myscull_read wait_event_interruptible wrong\n");
                return -ERESTARTSYS;
            }
        }
        
        if (copy_to_user(pbuf, pmydev->buf + *f_pos, count) != 0)
        {
            printk(KERN_ALERT "myscull_read: copy error!\n");
            return 0;
        }

        pmydev->size -= count;
        int i = 0;
        for (i = 0; i < pmydev->size; i++)
        {
            pmydev->buf[i] = pmydev->buf[count + i];
        }
        wake_up_interruptible(&pmydev->qwrite);
        printk(KERN_INFO "myscull_read: after read, size = %d\n", pmydev->size);
    }
    printk(KERN_INFO "myscull_read: leave count = %d, f_pos is %ld, filp->f_pos is %ld\n", count, *f_pos, filp->f_pos);
    return count;
}

static ssize_t myscull_write(struct file *filp, const char __user *pbuf, size_t count, loff_t *f_pos)
{
    struct myscull_dev* pmydev = (struct myscull_dev*)(filp->private_data);
    printk(KERN_INFO "myscull_write: enter capacity = %d, size = %ld, f_pos =%ld, filp->f_pos = %ld, count = %ld\n", 
        pmydev->capacity, pmydev->size, *f_pos, filp->f_pos, count);

    ssize_t length = (*f_pos + count < pmydev->capacity) ? count : (pmydev->capacity - *f_pos);
    ssize_t result = -ENOMEM;
    if (length > 0)
    {
        while (pmydev->size == pmydev->capacity)
        {
            if (wait_event_interruptible(pmydev->qwrite, pmydev->size != pmydev->capacity))
            {
                printk(KERN_INFO "myscull_write wait_event_interruptible wrong\n");
                return -ERESTARTSYS;
            }
        }

        if (copy_from_user(pmydev->buf, pbuf, length) != 0)
        {
            printk(KERN_ALERT "myscull_write: copy error!\n");
            return -EFAULT;
        }
        *f_pos += length;
        pmydev->size += length;
        result = length;
        printk(KERN_INFO "myscull_write: after write size = %ld, f_pos =%ld, filp->f_pos = %ld\n", pmydev->size, *f_pos, filp->f_pos);
        wake_up_interruptible(&pmydev->qread);
       
        if (pmydev->qsync)
            kill_fasync(&pmydev->qsync, SIGIO, POLL_IN);
    }
    return count;
}

static loff_t myscull_seek(struct file *filp, loff_t off, int whence)
{
    loff_t newpos;
    struct myscull_dev* pmydev = (struct myscull_dev*)(filp->private_data);

    switch(whence)
    {
        case 0:
            newpos = off;
            break;
        case 1:
            newpos = filp->f_pos + off;
            break;
        case 2:
           newpos = pmydev->size + off;
           break;
        default:
           return -EINVAL;
    }
    filp->f_pos = newpos;
    printk(KERN_INFO "myscull_seek: off is %ld, whence is %d, newpos is %ld\n", off, whence, newpos);
}

static long myscull_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    printk(KERN_INFO "enter myscull_ioctl cmd =%d\n");
    struct myscull_dev* pmydev = (struct myscull_dev*)(filp->private_data);
    switch (cmd)
    {
        case MYSCULL_IOC_SET_CAP:
            pmydev->capacity = arg;
            kfree(pmydev->buf);
            pmydev->buf = kmalloc(pmydev->capacity, GFP_KERNEL);
            break;
        default:
            return -ENOTTY;
    }
    return 0;
}

static int myscull_fasync(int fd, struct file *filp, int mode)
{
    printk(KERN_INFO "enter myscull_fasync\n");
    struct myscull_dev* pmydev = (struct myscull_dev*)(filp->private_data);
    return fasync_helper(fd, filp, mode, &pmydev->qsync);
}

struct file_operations myscull_ops = {
    .owner = THIS_MODULE,
    .read = myscull_read,
    .write = myscull_write,
    .open = myscull_open,
    .llseek = myscull_seek,
    .unlocked_ioctl = myscull_ioctl,
    .fasync = myscull_fasync,
};

dev_t myscull_dev_no;

static int myscull_init(void)
{
    int ret = 0;
    ret = alloc_chrdev_region(&myscull_dev_no, 0, 1, "myscull"); 
    if (ret < 0)
    {
        printk(KERN_WARNING "myscull: can't get major dev.");
        return ret;
    }

    cdev_init(&chardev1.my_cdev, &myscull_ops);
    chardev1.my_cdev.owner = THIS_MODULE;
    chardev1.size = 0;
    chardev1.capacity = 4; 
    chardev1.buf = kmalloc(chardev1.capacity, GFP_KERNEL);
    init_waitqueue_head(&chardev1.qread);
    init_waitqueue_head(&chardev1.qwrite);

    ret = cdev_add(&chardev1.my_cdev, myscull_dev_no, 1);
    if (ret)
    {
        printk(KERN_ALERT "can't add cdev\n");
        return ret;
    }

    printk(KERN_ALERT "Hello world: this is my scull!\n");
    return 0;
}

static void myscull_exit(void)
{
    kfree(chardev1.buf);
    unregister_chrdev_region(myscull_dev_no, 1);
    printk(KERN_ALERT "Goodbye, cruel world!\n");
}

module_init(myscull_init);
module_exit(myscull_exit);
