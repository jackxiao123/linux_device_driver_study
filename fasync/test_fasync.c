#include "myscull.h"
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>

int flag = 0;

void signal_handler(int num)
{
    if (num = SIGIO)
    {
     printf("singal is received: num = %d\n", num);
     flag = 1;
    }
}

int main(int argc, char** argv)
{
    int async_fd = open("/dev/myscull0", O_RDONLY);
    if (async_fd < 0)
    {
       printf("can not open /dev/myscull\n");
       return -1;
    }
    signal(SIGIO, signal_handler);
    fcntl(async_fd, F_SETOWN, getpid());
    int oflag = fcntl(async_fd, F_GETFL);
    fcntl(async_fd, F_SETFL, oflag|FASYNC);
    
    while (!flag);
    close(async_fd);

    return 0;
}
