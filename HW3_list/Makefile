KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)
CLANG_FORMAT ?= clang-format

SRC := $(shell find . -type f \( -name "*.c" -o -name "*.h" \))

.PHONY: make all clean format check

make: all

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

format:
	@command -v $(CLANG_FORMAT) >/dev/null || { \
		echo "ERROR: $(CLANG_FORMAT) not found. Install clang-format or set CLANG_FORMAT=clang-format-14"; \
		exit 127; \
	}
	$(CLANG_FORMAT) -i $(SRC)

check: all
	@test -x ./check.sh || { echo "ERROR: ./check.sh not executable (chmod +x check.sh)"; exit 1; }
	./check.sh
