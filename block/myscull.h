#include <linux/types.h>
#include <linux/ioctl.h>

#define MYSCULL_IOC_MAGIC 'r'
#define MYSCULL_IOC_SET_CAP  _IOW(MYSCULL_IOC_MAGIC, 0, int)
