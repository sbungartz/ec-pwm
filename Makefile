MODULE_NAME = ec_pwm

obj-m += $(MODULE_NAME).o

KVER := $(shell uname -r)
MODDESTDIR := /lib/modules/$(KVER)/kernel/drivers

all:
	make -C /lib/modules/$(KVER)/build M=$(PWD) modules

install:
	install -p -m 644 $(MODULE_NAME).ko  $(MODDESTDIR)
	/sbin/depmod -a $(KVER)

uninstall:
	rm -f $(MODDESTDIR)/$(MODULE_NAME).ko
	/sbin/depmod -a $(KVER)

clean:
	make -C /lib/modules/$(KVER)/build M=$(PWD) clean
