obj-m	+= lab1_semaphore1.o lab1_semaphore2.o lab1_semaphore3.o

KDIR	:= /lib/modules/$(shell uname -r)/build
PWD	:= $(shell pwd)


default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean


