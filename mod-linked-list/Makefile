obj-m   :=linked-list.o
KDIR    := /lib/modules/$(shell uname -r)/build
PWD     := $(shell pwd) 
SYM=$(PWD) 
 
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install

