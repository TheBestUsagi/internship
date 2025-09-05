# ==== toolchains ====
CC_x86   := gcc
CC_a64   := aarch64-linux-gnu-gcc
CC_armhf := arm-linux-gnueabihf-gcc

# QEMU & sysroot for running ARM binaries on x86
QEMU_A64 := qemu-aarch64-static
QEMU_ARM := qemu-arm-static
SYSROOT_A64 := /usr/aarch64-linux-gnu
SYSROOT_ARM := /usr/arm-linux-gnueabihf

# ==== build options ====
CFLAGS := -O2 -Wall
SRC    := $(wildcard src/*.c)

# ==== targets ====
.PHONY: all clean run-x86 run-aarch64 run-armhf proof help

all: app_x86 app_a64 app_armhf

app_x86: $(SRC)
	$(CC_x86)  $(CFLAGS) $^ -o $@

app_a64: $(SRC)
	$(CC_a64)  $(CFLAGS) $^ -o $@

app_armhf: $(SRC)
	$(CC_armhf) $(CFLAGS) $^ -o $@

run-x86: app_x86
	./$<

run-aarch64: app_a64
	$(QEMU_A64) -L $(SYSROOT_A64) ./$<

run-armhf: app_armhf
	$(QEMU_ARM) -L $(SYSROOT_ARM) ./$<

proof: all
	{ echo "== file =="; file app_x86 app_a64 app_armhf; \
	  echo; echo "== readelf aarch64 =="; readelf -h app_a64 | head -n 15; \
	  echo; echo "== readelf armhf ==";   readelf -h app_armhf | head -n 15; } | tee proof.txt

clean:
	rm -f app_x86 app_a64 app_armhf proof.txt

help:
	@echo "make            # build x86/ARM64/ARM32"
	@echo "make run-x86    # run x86 binary"
	@echo "make run-aarch64# run ARM64 via QEMU"
	@echo "make run-armhf  # run ARM32 via QEMU"
	@echo "make proof      # generate proof.txt"
	@echo "make clean      # remove outputs"
