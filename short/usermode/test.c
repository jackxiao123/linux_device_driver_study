#include <stdio.h>
#include <unistd.h>
#include <sys/io.h>

#define BASEPORT 0x378 /* lp1 */

int main()
{
  if (ioperm(BASEPORT, 3, 1))
  {
     perror("ioperm"); exit(1);
  }
  
  outb(0, BASEPORT);
  
  usleep(100000);
  
  printf("status: %d\n", inb(BASEPORT + 1));

  if (ioperm(BASEPORT, 3, 0)) 
  {
     perror("ioperm"); exit(1);
  }

  exit(0);
}
