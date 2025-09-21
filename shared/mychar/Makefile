# ===== 内核模块 =====
obj-m += mychar.o

# 目标架构：armv7l
ARCH ?= arm
CROSS_COMPILE ?= arm-linux-gnueabihf-

# 指向“QEMU 所跑内核(6.12.0)”对应的 build 目录（里面必须有 Module.symvers）
# 没有就手动改成你那套内核源码编译输出

KDIR ?= /home/usagi/projects/linux-kernel-qemu/kernel/linux/build
ARCH ?= arm
CROSS_COMPILE ?= arm-linux-gnueabihf-


PWD := $(shell pwd)

# ===== 用户态工具（ioctl 小程序 myctl）=====
# 用 armv7 交叉编译器编出能在 QEMU 里直接跑的二进制
HOSTCC ?= $(CROSS_COMPILE)gcc
# 静态链接最省事（虚机里没库也能跑）；如果链不起来，去掉 -static
HOSTCFLAGS ?= -O2 -static -s

.PHONY: all clean

all: mychar.ko myctl

mychar.ko:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) modules

myctl: myctl.c
	$(HOSTCC) $(HOSTCFLAGS) $< -o $@

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) clean
	$(RM) myctl

