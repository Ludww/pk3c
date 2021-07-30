MODULENAME=tcp_pk3c
SRCS=$(MODULENAME)_main.c proc_version.c
OBJS=$(SRCS:.c=.o)
obj-m += $(MODULENAME).o
$(MODULENAME)-objs := $(OBJS)

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
clean:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) clean
