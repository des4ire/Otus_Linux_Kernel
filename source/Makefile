KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)
SRC  := $(wildcard *.c)

.PHONY: all clean format check


ifneq ($(KERNELRELEASE),)

include $(src)/Kbuild

else

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

format:
	clang-format -i $(SRC)

check: all
	./check.sh

endif
