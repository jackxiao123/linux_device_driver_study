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
#include <linux/poll.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("jackx");
MODULE_DESCRIPTION("A simple scull module");

struct myscull1_dev
{
    struct cdev my_cdev;
    char* buf;
    int size;
    int capacity;
    wait_queue_head_t qread;
    wait_queue_head_t qwrite;
} chardev1;

static int myscull1_open(struct inode *inodp, struct file *filp)
{
    struct cdev* pcdev = inodp->i_cdev;
    struct myscull1_dev* pmydev = container_of(pcdev, struct myscull1_dev, my_cdev);
    filp->private_data = pmydev;
    printk(KERN_INFO "myscull1_open: capacity=%d, size=%d, f_pos=%ld\n", pmydev->capacity, pmydev->size, filp->f_pos);
    return 0;
}

static ssize_t myscull1_read(struct file *filp, char __user *pbuf, size_t count, loff_t *f_pos)
{
    struct myscull1_dev* pmydev = (struct myscull1_dev*)(filp->private_data);
    printk(KERN_INFO "myscull1_read: enter count is %ld, size = %d, f_pos is %ld, filp->f_pos is %ld\n", count, pmydev->size, *f_pos, filp->f_pos);
    count = (*f_pos + count < pmydev->size) ? count : (pmydev->size - *f_pos);
    if (count > 0)
    {
        while (pmydev->size == 0)
        {
            //if (wait_event(pmydev->qread, pmydev->size != 0))
            if (wait_event_interruptible(pmydev->qread, pmydev->size != 0))
            {
                printk(KERN_INFO "myscull1_read wait_event_interruptible wrong\n");
                return -ERESTARTSYS;
            }
        }
        
        if (copy_to_user(pbuf, pmydev->buf + *f_pos, count) != 0)
        {
            printk(KERN_ALERT "myscull1_read: copy error!\n");
            return 0;
        }

        pmydev->size -= count;
        int i = 0;
        for (i = 0; i < pmydev->size; i++)
        {
            pmydev->buf[i] = pmydev->buf[count + i];
        }
        //wake_up(&pmydev->qwrite);
        wake_up_interruptible(&pmydev->qwrite);
        printk(KERN_INFO "myscull1_read: after read, size = %d\n", pmydev->size);
    }
    printk(KERN_INFO "myscull1_read: leave count = %d, f_pos is %ld, filp->f_pos is %ld\n", count, *f_pos, filp->f_pos);
    return count;
}

static ssize_t myscull1_write(struct file *filp, const char __user *pbuf, size_t count, loff_t *f_pos)
{
    struct myscull1_dev* pmydev = (struct myscull1_dev*)(filp->private_data);
    printk(KERN_INFO "myscull1_write: enter capacity = %d, size = %ld, f_pos =%ld, filp->f_pos = %ld, count = %ld\n", 
        pmydev->capacity, pmydev->size, *f_pos, filp->f_pos, count);

    ssize_t length = (*f_pos + count < pmydev->capacity) ? count : (pmydev->capacity - *f_pos);
    ssize_t result = -ENOMEM;
    if (length > 0)
    {
        while (pmydev->size == pmydev->capacity)
        {
            //if (wait_event(pmydev->qwrite, pmydev->size != pmydev->capacity))
            if (wait_event_interruptible(pmydev->qwrite, pmydev->size != pmydev->capacity))
            {
                printk(KERN_INFO "myscull1_write wait_event_interruptible wrong\n");
                return -ERESTARTSYS;
            }
        }

        if (copy_from_user(pmydev->buf, pbuf, length) != 0)
        {
            printk(KERN_ALERT "myscull1_write: copy error!\n");
            return -EFAULT;
        }
        *f_pos += length;
        pmydev->size += length;
        result = length;
        printk(KERN_INFO "myscull1_write: after write size = %ld, f_pos =%ld, filp->f_pos = %ld\n", pmydev->size, *f_pos, filp->f_pos);
        //wake_up(&pmydev->qread);
        wake_up_interruptible(&pmydev->qread);
    }
    return count;
}

static loff_t myscull1_seek(struct file *filp, loff_t off, int whence)
{
    loff_t newpos;
    struct myscull1_dev* pmydev = (struct myscull1_dev*)(filp->private_data);

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
    printk(KERN_INFO "myscull1_seek: off is %ld, whence is %d, newpos is %ld\n", off, whence, newpos);
}

static long myscull1_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    printk(KERN_INFO "enter myscull1_ioctl cmd =%d\n", cmd);
    struct myscull1_dev* pmydev = (struct myscull1_dev*)(filp->private_data);
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

static unsigned int myscull1_poll(struct file *filp, poll_table *wait)
{
    printk(KERN_INFO "enter myscull1_poll\n");
    struct myscull1_dev* pmydev = (struct myscull1_dev*)(filp->private_data);
    unsigned int mask = 0;

    poll_wait(filp, &pmydev->qread, wait);
    poll_wait(filp, &pmydev->qwrite, wait);

    if (pmydev->size > 0)
        mask |= POLLIN | POLLRDNORM; // readable
    if (pmydev->size == 0)
        mask |= POLLOUT | POLLWRNORM; // writable
    return mask;
}

struct file_operations myscull1_ops = {
    .owner = THIS_MODULE,
    .read = myscull1_read,
    .write = myscull1_write,
    .open = myscull1_open,
    .llseek = myscull1_seek,
    .unlocked_ioctl = myscull1_ioctl,
    .poll = myscull1_poll,
};

dev_t myscull1_dev_no;

static int myscull1_init(void)
{
    int ret = 0;
    ret = alloc_chrdev_region(&myscull1_dev_no, 0, 1, "myscull1"); 
    if (ret < 0)
    {
        printk(KERN_WARNING "myscull1: can't get major dev.");
        return ret;
    }

    cdev_init(&chardev1.my_cdev, &myscull1_ops);
    chardev1.my_cdev.owner = THIS_MODULE;
    chardev1.size = 0;
    chardev1.capacity = 4; 
    chardev1.buf = kmalloc(chardev1.capacity, GFP_KERNEL);
    init_waitqueue_head(&chardev1.qread);
    init_waitqueue_head(&chardev1.qwrite);

    ret = cdev_add(&chardev1.my_cdev, myscull1_dev_no, 1);
    if (ret)
    {
        printk(KERN_ALERT "can't add cdev\n");
        return ret;
    }

    printk(KERN_ALERT "Hello world: this is my scull 1!\n");
    return 0;
}

static void myscull1_exit(void)
{
    kfree(chardev1.buf);
    unregister_chrdev_region(myscull1_dev_no, 1);
    printk(KERN_ALERT "Goodbye, cruel world myscull1!\n");
}

module_init(myscull1_init);
module_exit(myscull1_exit);
