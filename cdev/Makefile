ifneq ($(KERNELRELEASE),)
obj-m := trivMod.o
else
ARCH ?= arm64
CROSS_COMPILE ?= aarch64-linux-
KDIR ?= /home/$(USER)/rpi-buildroot-2026.08/output/build/linux-custom
default:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(CURDIR) modules
clean:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(CURDIR) clean
endif
