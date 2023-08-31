CONFIG_MODULE_SIG=n
CONFIG_MODULE_SIG_ALL=n

obj-m += ipc-so.o
CFLAGS_ipc-so.o := -std=gnu99

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	sudo dmesg --clear
	make -C $(KDIR) M=$(PWD) clean
	clear

insert:
	sudo dmesg --clear
	clear
	sudo insmod ipc-so.ko
	sudo chmod 777 /dev/registration
	sudo chmod 777 /dev/shared_memory
	sudo chmod 777 /dev/synchronous
	dmesg

list:
	lsmod

delete:
	sudo rmmod ipc-so.ko
	dmesg