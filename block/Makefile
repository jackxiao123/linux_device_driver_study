obj-m += myscull.o
all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
	gcc setcap.c -o setcap
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
