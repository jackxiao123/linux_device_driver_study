/*
 *  * jit.c -- the just-in-time module
 *  *
 *  * Copyright (C) 2001,2003 Alessandro Rubini and Jonathan Corbet
 *  * Copyright (C) 2001,2003 O'Reilly & Associates
 *  *
 *  * The source code in this file can be freely used, adapted,
 *  * and redistributed in source or binary form, so long as an
 *  * acknowledgment appears in derived source files. The citation
 *  * should list that the code comes from the book "Linux Device
 *  * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 *  * by O'Reilly & Associates. No warranty is attached;
 *  * we cannot take responsibility for errors or fitness for use.
 *  *
 *  */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("jackx");
MODULE_DESCRIPTION("A hello world just-in-time module");

static int myjit_current_time(struct seq_file *m, void *v)
{
    struct timeval tv1;
    struct timespec tv2;
    unsigned long j1;
    u64 j2;

    j1 = jiffies;
    j2 = get_jiffies_64();
    do_gettimeofday(&tv1);
    tv2 = current_kernel_time();

    seq_printf(m, "jiffies is %ld\n", j1);
    seq_printf(m, "get_jiffies_64 is %ld\n", j2);
    seq_printf(m, "do_gettimeofday is %ld:%ld\n", tv1.tv_sec, tv1.tv_usec);
    seq_printf(m, "current_kernel_time is %ld:%ld\n", tv2.tv_sec, tv2.tv_nsec);
    return 0;
}

static int myjit_open(struct inode *inode, struct file *filp)
{
    return single_open(filp, myjit_current_time, NULL);
}

static struct file_operations myops =
{
    .owner = THIS_MODULE,
    .open = myjit_open,
    .read = seq_read,
    .release = single_release,
};

static int hello_init(void)
{
    printk(KERN_ALERT "Hello, world - just in time\n");

    struct proc_dir_entry* parent = proc_mkdir("myjit", NULL);
    proc_create("currentime", 0, parent, &myops);

    return 0;
}

static void hello_exit(void)
{
    printk(KERN_ALERT "Goodbye, cruel world - just in time\n");
}

module_init(hello_init);
module_exit(hello_exit);
