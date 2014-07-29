#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/acpi.h>

struct fanval_attr {
	struct attribute attr;
	unsigned int ecreg;
	unsigned int value;
	struct fanval_attr *ecattr;
};

static struct fanval_attr fan_left = {
	.attr.name = "left",
	.attr.mode = 0666,
	.ecreg = 243,
	.value = 0,
	.ecattr = &fan_left,
};

static struct fanval_attr fan_left_pwm = {
	.attr.name = "left_pwm",
	.attr.mode = 0666,
	.ecattr = &fan_left,
};

static struct fanval_attr fan_right = {
	.attr.name = "right",
	.attr.mode = 0666,
	.ecreg = 242,
	.value = 0,
	.ecattr = &fan_right,
};

static struct fanval_attr fan_right_pwm = {
	.attr.name = "right_pwm",
	.attr.mode = 0666,
	.ecattr = &fan_right,
};

static struct attribute* fan_attrs[] = { &fan_left.attr, &fan_left_pwm.attr, &fan_right.attr, &fan_right_pwm.attr, NULL };

static ssize_t fanval_show(struct kobject *kobj, struct attribute *attr, char *buf) {
	struct fanval_attr *a = container_of(attr, struct fanval_attr, attr);
	struct fanval_attr *ec = a->ecattr;

	u8 valbuf;
	
	ec_read(ec->ecreg, &valbuf);
	ec->value = valbuf;

	if(a != ec) { // This is a PWM attr
		printk(KERN_DEBUG "read pwm\n");
		a->value = 255 - ec->value;
	}

	return scnprintf(buf, PAGE_SIZE, "%u\n", a->value);
}

static ssize_t fanval_store(struct kobject *kobj, struct attribute *attr, const char *buf, size_t len) {
	struct fanval_attr *a = container_of(attr, struct fanval_attr, attr);
	struct fanval_attr *ec = a->ecattr;

	sscanf(buf, "%u", &a->value);

	if(a != ec) { // This is a PWM attr
		printk(KERN_DEBUG "write pwm\n");
		// In order to avoid damage to the fans from spinning to fast, we yield control to the Embedded Controller (by setting PWM to 255),
		// if the desired speed becomse to high.
		if(a->value > 120) {
			a->value = 255;
		}
		ec->value = 255 - a->value;
	}

	printk(KERN_DEBUG "Write: %u to %u\n", ec->value, ec->ecreg);
	ec_write(ec->ecreg, ec->value);

	return sizeof(int);
}

static struct sysfs_ops fanval_ops = {
	.show = fanval_show,
	.store = fanval_store,
};

static struct kobj_type fanval_type = {
	.sysfs_ops = &fanval_ops,
	.default_attrs = fan_attrs,
};

struct kobject *fanvalobj;

int init_module(void) {
	int err;

	err = -1;
	fanvalobj = kzalloc(sizeof(*fanvalobj), GFP_KERNEL);
	if(fanvalobj) {
		kobject_init(fanvalobj, &fanval_type);
		if(kobject_add(fanvalobj, &THIS_MODULE->mkobj.kobj, "%s", "fans")) {
			err = -1;
			printk("Sysfs creation failed\n");
			kobject_put(fanvalobj);
			fanvalobj = NULL;
		}
		err = 0;
	}

	return err;
}

void cleanup_module(void) {
	// Reset fans so they are controlled by Embedded Controller.
	ec_write(fan_left.ecreg, 0);
	ec_write(fan_right.ecreg, 0);

	if(fanvalobj) {
		kobject_put(fanvalobj);
		kfree(fanvalobj);
	}
}
