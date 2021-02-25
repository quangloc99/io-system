obj-m += io_lab1.o

all:
	make -C /lib/modules/`uname -r`/build M=$(PWD) modules
clean:
	make -C /lib/modules/`uname -r`/build M=$(PWD) clean
	
install:
	sudo insmod io_lab1.ko
	
remove:
	sudo rmmod io_lab1

test_print_big: test_print_big.c
	gcc test_print_big.c -o test_print_big
	
do_test_print_big: test_print_big
	./test_print_big
