#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("jackx");
MODULE_DESCRIPTION("A simple scull module");

struct myscull_dev
{
    struct cdev my_cdev;
    char *buf;
    int size;
} chardev1;

static int myscull_open(struct inode *inodp, struct file *filp)
{
    struct cdev* pcdev = inodp->i_cdev;
    struct myscull_dev* pmydev = container_of(pcdev, struct myscull_dev, my_cdev);
    filp->private_data = pmydev;
    printk(KERN_INFO "myscull_open: size = %d\n", pmydev->size);
    return 0;
}

static ssize_t myscull_read(struct file *filp, char __user *pbuf, size_t count, loff_t *f_pos)
{
    struct myscull_dev* pmydev = (struct myscull_dev*)(filp->private_data);
    count = (*f_pos + count < pmydev->size) ? count : (pmydev->size - *f_pos);
    if (count > 0)
    {
        if (copy_to_user(pbuf, pmydev->buf + *f_pos, count) != 0)
        {
            printk(KERN_ALERT "myscull_read: copy error!\n");
            return 0;
        }
    }
    *f_pos += count;
    return count;
}

static ssize_t myscull_write(struct file *filp, const char __user *pbuf, size_t count, loff_t *f_pos)
{
    struct myscull_dev* pmydev = (struct myscull_dev*)(filp->private_data);
    printk(KERN_INFO "myscull_write: start writing, size = %ld\n", count);
    if (copy_from_user(pmydev->buf, pbuf, count) != 0)
    {
        printk(KERN_ALERT "myscull_write: copy error!\n");
        return 0;
    }
    *f_pos += count;
    pmydev->size += count;
    return count;
}

struct file_operations myscull_ops = {
    .owner = THIS_MODULE,
    .read = myscull_read,
    .write = myscull_write,
    .open = myscull_open,
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
    unregister_chrdev_region(myscull_dev_no, 1);
    printk(KERN_ALERT "Goodbye, cruel world!\n");
}

module_init(myscull_init);
module_exit(myscull_exit);
