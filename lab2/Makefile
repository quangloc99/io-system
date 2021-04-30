obj-m += io_lab2.o
obj-m += io_lab2_var4.o

MY_CFLAGS += -g -DDEBUG
ccflags-y += ${MY_CFLAGS}
CC += ${MY_CFLAGS}

all:
	make -C /lib/modules/`uname -r`/build M=$(PWD) modules
	EXTRA_CFLAGS="$(MY_CFLAGS)"
	
clean:
	make -C /lib/modules/`uname -r`/build M=$(PWD) clean
	
install:
	sudo insmod io_lab1.ko
	
remove:
	sudo rmmod io_lab1

