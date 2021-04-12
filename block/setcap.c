#include "myscull.h"
#include <fcntl.h>
#include <stdio.h>

int main(int argc, char** argv)
{
    if (argc < 2) 
    {
        printf("wrong argument count, exit.\n");
        return 1;
    }
    int num = atoi(argv[1]);
    int fd = open("/dev/myscull0", O_RDWR);
    ioctl(fd, MYSCULL_IOC_SET_CAP, num);
    printf("set cap to %d\n", num);
    close(fd);
    return 0;
}
