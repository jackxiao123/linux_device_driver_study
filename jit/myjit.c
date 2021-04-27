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
#include <linux/sched.h>
#include <linux/wait.h>

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

static int myjit_current_time_open(struct inode *inode, struct file *filp)
{
    return single_open(filp, myjit_current_time, NULL);
}

static struct file_operations mypos_current_time =
{
    .owner = THIS_MODULE,
    .open = myjit_current_time_open,
    .read = seq_read,
    .release = single_release,
};

static int myjit_busy(struct seq_file *m, void *v)
{
    unsigned long j1 = 0;
    seq_printf(m, "before busy 10s:%ld\n", jiffies);
    j1 = jiffies + 10000;
    while (time_before(jiffies, j1)) cpu_relax();
    seq_printf(m, "after busy 10s:%ld\n", jiffies);
    return 0;
}

static int myjit_busy_open(struct inode *inode, struct file *filp)
{
    return single_open(filp, myjit_busy, NULL);
}

static struct file_operations mypos_busy =
{
    .owner = THIS_MODULE,
    .open = myjit_busy_open,
    .read = seq_read,
    .release = single_release,
};

static int myjit_schedule(struct seq_file *m, void *v)
{
    seq_printf(m, "before schedule 10s:%ld\n", jiffies);
    unsigned long j1 = jiffies + 10000;
    while (time_before(jiffies, j1)) schedule();
    seq_printf(m, "after schedule 10s:%ld\n", jiffies);
    return 0;
}

static int myjit_schedule_open(struct inode *inode, struct file *filp)
{
    return single_open(filp, myjit_schedule, NULL);
}

static struct file_operations mypos_schedule =
{
    .owner = THIS_MODULE,
    .open = myjit_schedule_open,
    .read = seq_read,
    .release = single_release,
};

static int myjit_queue(struct seq_file *m, void *v)
{
    seq_printf(m, "before queue 10s:%ld\n", jiffies);
    //wait_queue_head_t wait;
    //init_waitqueue_head(&wait);
    //wait_event_interruptible_timeout(wait, 0, HZ * 10);
    set_current_state(TASK_INTERRUPTIBLE);
    schedule_timeout(HZ*10);
    seq_printf(m, "after queue 10s:%ld\n", jiffies);
    return 0;
}

static int myjit_queue_open(struct inode *inode, struct file *filp)
{
    return single_open(filp, myjit_queue, NULL);
}

static struct file_operations mypos_queue =
{
    .owner = THIS_MODULE,
    .open = myjit_queue_open,
    .read = seq_read,
    .release = single_release,
};

static struct proc_dir_entry * parent = NULL;

static int hello_init(void)
{
    printk(KERN_ALERT "Hello, world - just in time\n");

    parent = proc_mkdir("myjit", NULL);
    proc_create("currentime", 0, parent, &mypos_current_time);
    proc_create("busy", 0, parent, &mypos_busy);
    proc_create("sched", 0, parent, &mypos_schedule);
    proc_create("queue", 0, parent, &mypos_queue);

    return 0;
}

static void hello_exit(void)
{
    remove_proc_entry("currentime", parent);
    remove_proc_entry("busy", parent);
    remove_proc_entry("sched", parent);
    remove_proc_entry("queue", parent);
    proc_remove(parent);
    printk(KERN_ALERT "Goodbye, cruel world - just in time\n");
}

module_init(hello_init);
module_exit(hello_exit);
