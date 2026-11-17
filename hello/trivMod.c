#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Trivial hello world kernel module");
MODULE_AUTHOR("dororiya");

static int __init mymod_init(void)
{
	printk(KERN_INFO "hello world\n");
	return 0;
}

static void __exit mymod_exit(void)
{
	printk(KERN_INFO "bye bye world\n");
}

module_init(mymod_init);
module_exit(mymod_exit);
