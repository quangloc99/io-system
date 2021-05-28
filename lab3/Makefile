obj-m :=virt_net_if.o
all :
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
clean :
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
	
reinstall: all
	sudo rmmod virt_net_if
	sudo insmod virt_net_if.ko
	sudo ip link set enp0s3 up
