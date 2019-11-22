obj-m += seconds.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean: remove
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

install: all
	sudo insmod seconds.ko

test: all install
	cat /proc/seconds

remove: all
	sudo rmmod seconds