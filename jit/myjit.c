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
#include <linux/interrupt.h>
#include <linux/delay.h>

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
    seq_printf(m, "get_jiffies_64 is %ld\n", (long)j2);
    seq_printf(m, "do_gettimeofday is %ld:%ld\n", tv1.tv_sec, tv1.tv_usec);
    seq_printf(m, "current_kernel_time is %ld:%ld\n", tv2.tv_sec, tv2.tv_nsec);

    j1 = jiffies;
    j2 = get_jiffies_64();
    do_gettimeofday(&tv1);
    tv2 = current_kernel_time();
    seq_printf(m, "jiffies is %ld\n", j1);
    seq_printf(m, "get_jiffies_64 is %ld\n", (long)j2);
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
    seq_printf(m, "before busy 10s:%ld cpu is %d\n", jiffies, smp_processor_id());
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
    unsigned long j1;
    seq_printf(m, "before schedule 10s:%ld\n", jiffies);
    j1 = jiffies + 10000;
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

#define JIT_ASYNC_LOOPS 20 
#define JIT_DELAY 1000

struct jitimer_data
{
    unsigned long prev_jiffies;
    struct seq_file *m;
    struct timer_list t;
    wait_queue_head_t qwait;
    int count;
};

static struct jitimer_data my_timer_data;

static void jit_timer_fn(struct timer_list *t)
{
    struct jitimer_data *data = container_of(t, struct jitimer_data, t); 
    unsigned long now = jiffies;

    seq_printf(data->m, "%10ld %6ld %6ld %9d %9d %3d %-30s\n",
        now, (long)(now - data->prev_jiffies), in_interrupt(), in_atomic(), task_pid_nr(current), smp_processor_id(), current->comm);
    data->count--;
    if (data->count > 0)
    {
        data->prev_jiffies = now;
        mod_timer(&data->t, now + JIT_DELAY);
    }
    else
    {
        wake_up_interruptible(&data->qwait);
    }
}

static int myjit_timer(struct seq_file *m, void *v)
{
    int ret;
    unsigned long now = jiffies;
   
    timer_setup(&my_timer_data.t, jit_timer_fn, 0);
    my_timer_data.t.expires = now + JIT_DELAY;
    my_timer_data.prev_jiffies = now;
    my_timer_data.m = m;
    my_timer_data.count = JIT_ASYNC_LOOPS;

    seq_printf(m, "%10s %6s %6s %9s %9s %3s %-30s\n",
        "time", "delta", "inirq", "inatomic", "pid", "cpu", "cmd");
    seq_printf(m, "%10ld %6d %6ld %9d %9d %3d %-30s\n",
        now, 0, in_interrupt(), in_atomic(), task_pid_nr(current), smp_processor_id(), current->comm);

    add_timer(&my_timer_data.t);

    while (my_timer_data.count > 0)
    {
        if (wait_event_interruptible(my_timer_data.qwait, my_timer_data.count == 0))
        {
            printk("myjit wait_event_interruptible fail\n");
            ret = -ERESTARTSYS;
            goto cleanup;
        }
    }
    ret = 0;
cleanup:
   my_timer_data.count = 0;
    del_timer_sync(&my_timer_data.t);
    return ret;
}

static int myjit_timer_open(struct inode *inode, struct file *filp)
{
    return single_open(filp, myjit_timer, NULL);
}

static struct file_operations mypos_timer =
{
    .owner = THIS_MODULE,
    .open = myjit_timer_open,
    .read = seq_read,
    .release = single_release,
};

struct jitasklet_data
{
    unsigned long prev_jiffies;
    struct seq_file *m;
    struct tasklet_struct t;
    wait_queue_head_t qwait;
    int count;
};

static struct jitasklet_data my_tasklet_data;

static void jit_tasklet_fn(unsigned long arg)
{
    struct jitasklet_data *data = (struct jitasklet_data *)arg; 
    unsigned long now = jiffies;

    seq_printf(data->m, "%10ld %6ld %6ld %9d %9d %3d %-30s\n",
        now, (long)(now - data->prev_jiffies), in_interrupt(), in_atomic(), task_pid_nr(current), smp_processor_id(), current->comm);
    data->count--;
    if (data->count > 0)
    {
        data->prev_jiffies = now;
        tasklet_schedule(&data->t);
    }
    else
    {
        wake_up_interruptible(&data->qwait);
    }
}

static int myjit_tasklet(struct seq_file *m, void *v)
{
    int ret;
    unsigned long now = jiffies;
   
    tasklet_init(&my_tasklet_data.t, jit_tasklet_fn, (unsigned long)(&my_tasklet_data));
    my_tasklet_data.prev_jiffies = now;
    my_tasklet_data.m = m;
    my_tasklet_data.count = JIT_ASYNC_LOOPS;

    seq_printf(m, "%10s %6s %6s %9s %9s %3s %-30s\n",
        "time", "delta", "inirq", "inatomic", "pid", "cpu", "cmd");
    seq_printf(m, "%10ld %6d %6ld %9d %9d %3d %-30s\n",
        now, 0, in_interrupt(), in_atomic(), task_pid_nr(current), smp_processor_id(), current->comm);

    msleep_interruptible(3000);

    tasklet_schedule(&my_tasklet_data.t);

    while (my_tasklet_data.count > 0)
    {
        if (wait_event_interruptible(my_tasklet_data.qwait, my_tasklet_data.count == 0))
        {
            printk("myjit_tasklet wait_event_interruptible fail\n");
            ret = -ERESTARTSYS;
            goto cleanup;
        }
    }
    ret = 0;
cleanup:
    my_tasklet_data.count = 0;
    tasklet_kill(&my_tasklet_data.t);
    return ret;
}

static int myjit_tasklet_open(struct inode *inode, struct file *filp)
{
    return single_open(filp, myjit_tasklet, NULL);
}

static struct file_operations mypos_tasklet =
{
    .owner = THIS_MODULE,
    .open = myjit_tasklet_open,
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
    proc_create("timer", 0, parent, &mypos_timer);
    proc_create("tasklet", 0, parent, &mypos_tasklet);

    init_waitqueue_head(&my_timer_data.qwait);
    init_waitqueue_head(&my_tasklet_data.qwait);

    return 0;
}

static void hello_exit(void)
{
    remove_proc_entry("currentime", parent);
    remove_proc_entry("busy", parent);
    remove_proc_entry("sched", parent);
    remove_proc_entry("queue", parent);
    remove_proc_entry("timer", parent);
    proc_remove(parent);
    printk(KERN_ALERT "Goodbye, cruel world - just in time\n");
}

module_init(hello_init);
module_exit(hello_exit);
