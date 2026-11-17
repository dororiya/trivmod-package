#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Trivial character device with a device file");
MODULE_AUTHOR("dororiya");

static dev_t dev;
static struct cdev my_cdev;
static struct class *my_class;

static struct file_operations my_fops = {
	.owner = THIS_MODULE,
};

static int __init mymod_init(void)
{
	if (alloc_chrdev_region(&dev, 0, 1, "trivmod") < 0) {
		printk(KERN_ERR "trivmod: failed to get device number\n");
		return -1;
	}

	cdev_init(&my_cdev, &my_fops);
	if (cdev_add(&my_cdev, dev, 1) < 0) {
		printk(KERN_ERR "trivmod: failed to add cdev\n");
		goto err_region;
	}

	my_class = class_create("trivclass");
	if (IS_ERR(my_class)) {
		printk(KERN_ERR "trivmod: failed to create class\n");
		goto err_cdev;
	}

	if (IS_ERR(device_create(my_class, NULL, dev, NULL, "trivdev"))) {
		printk(KERN_ERR "trivmod: failed to create device\n");
		goto err_class;
	}

	printk(KERN_INFO "hello world, major = %d\n", MAJOR(dev));
	return 0;

err_class:
	class_destroy(my_class);
err_cdev:
	cdev_del(&my_cdev);
err_region:
	unregister_chrdev_region(dev, 1);
	return -1;
}

static void __exit mymod_exit(void)
{
	device_destroy(my_class, dev);
	class_destroy(my_class);
	cdev_del(&my_cdev);
	unregister_chrdev_region(dev, 1);
	printk(KERN_INFO "bye bye world\n");
}

module_init(mymod_init);
module_exit(mymod_exit);
