#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Trivial character device module");
MODULE_AUTHOR("dororiya");

static dev_t dev;
static struct cdev my_cdev;

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
		unregister_chrdev_region(dev, 1);
		return -1;
	}

	printk(KERN_INFO "hello world, major = %d\n", MAJOR(dev));
	return 0;
}

static void __exit mymod_exit(void)
{
	cdev_del(&my_cdev);
	unregister_chrdev_region(dev, 1);
	printk(KERN_INFO "bye bye world\n");
}

module_init(mymod_init);
module_exit(mymod_exit);
