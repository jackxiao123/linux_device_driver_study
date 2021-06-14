#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>

int main(int argc, char** argv)
{
    int fd = 0;
    if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0)
    {
	printf("Can't open /dev/mem: %d\n", errno);
	return -1;
    }

    volatile unsigned int* gpio = (unsigned int*)mmap(0, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x3f200000);
    if (!gpio)
    {
	printf("mmap failure.\n");    
	return -1;
    }
    else
    {
	printf("gpio VA is %p\n", gpio);    
    }

    volatile unsigned int* GPFSEL0 = gpio;
    // set pin 4 as output
    *GPFSEL0 &= ~(0x6 << 12);
    *GPFSEL0 |= (0x1 << 12);
    // set pin 5 as output
    *GPFSEL0 &= ~(0x6 << 15);
    *GPFSEL0 |= (0x1 << 15);

    volatile unsigned int* GPSET0 = gpio + 7; 
    volatile unsigned int* GPCLR0 = gpio + 10; 

    int count = 10;
    while (count)
    {
	if (count % 2)    
	{
            *GPSET0 |= 0x1 << 4; 		
            *GPSET0 |= 0x1 << 5; 		
	}
	else
	{
            *GPCLR0 |= 0x1 << 4;		
            *GPCLR0 |= 0x1 << 5;		
	}
	sleep(1);
	count --;
    }

    *GPCLR0 |= 0x1 << 4;		

    munmap((void*)gpio, getpagesize());
     
    return 0;
}
