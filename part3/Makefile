obj-m += elevator.o
elevator-objs := src/elevator_main.o src/elevator_thread.o src/elevator_proc.o

KDIR := /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) clean

.PHONY: all clean