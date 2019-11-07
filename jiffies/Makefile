obj-m += jiffies.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean: remove
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

install: all
	sudo insmod jiffies.ko

test: all install
	watch -n 1 cat /proc/jiffies

remove: all
	sudo rmmod jiffies