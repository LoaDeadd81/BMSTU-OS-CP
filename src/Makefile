obj-m += netstat.o

KDIR ?= /lib/modules/$(shell uname -r)/build

ccflags-y += -std=gnu18 -Wall

build:
	make -C $(KDIR) M=$(shell pwd) modules

clean:
	make -C $(KDIR) M=$(shell pwd) clean

ins: build
	sudo insmod netstat.ko

rm: 
	sudo rmmod netstat

ri: rm ins