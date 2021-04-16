obj-m += myscull.o
all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
	gcc test_fasync.c -o test_fasync 
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
