#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Trivial character device with open and release");
MODULE_AUTHOR("dororiya");

/* our private data: the cdev is embedded inside it */
struct trivmod_dev {
    struct cdev cdev;
    int open_count;      /* how many times opened */
    int release_count;   /* how many times closed */
    int users;           /* how many are open right now */
};

static dev_t dev;
static struct trivmod_dev my_dev;   /* statically allocated */
static struct class *my_class;

static int my_open(struct inode *inode, struct file *file)
{
    struct trivmod_dev *d;

    printk(KERN_INFO "trivmod: open called\n");

    /* find our structure from the cdev inside it */
    d = container_of(inode->i_cdev, struct trivmod_dev, cdev);
    printk(KERN_INFO "trivmod: found private data with container_of\n");

    /* save it so release (and later read/write) can use it */
    file->private_data = d;
    printk(KERN_INFO "trivmod: private_data set\n");

    /* update statistics */
    d->open_count++;
    d->users++;
    printk(KERN_INFO "trivmod: stats: opens=%d closes=%d users=%d\n",
           d->open_count, d->release_count, d->users);

    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    struct trivmod_dev *d = file->private_data;

    printk(KERN_INFO "trivmod: release called\n");

    /* update statistics */
    d->release_count++;
    d->users--;
    printk(KERN_INFO "trivmod: stats: opens=%d closes=%d users=%d\n",
           d->open_count, d->release_count, d->users);

    if (d->users == 0)
        printk(KERN_INFO "trivmod: last user closed the device\n");

    return 0;
}

static struct file_operations my_fops = {
    .owner   = THIS_MODULE,
    .open    = my_open,
    .release = my_release,
};

static int __init mymod_init(void)
{
    if (alloc_chrdev_region(&dev, 0, 1, "trivmod") < 0) {
        printk(KERN_ERR "trivmod: failed to get device number\n");
        return -1;
    }
    printk(KERN_INFO "trivmod: got major %d\n", MAJOR(dev));

    cdev_init(&my_dev.cdev, &my_fops);
    if (cdev_add(&my_dev.cdev, dev, 1) < 0) {
        printk(KERN_ERR "trivmod: failed to add cdev\n");
        goto err_region;
    }
    printk(KERN_INFO "trivmod: cdev added\n");

    my_class = class_create("trivclass");
    if (IS_ERR(my_class)) {
        printk(KERN_ERR "trivmod: failed to create class\n");
        goto err_cdev;
    }
    printk(KERN_INFO "trivmod: class created\n");

    if (IS_ERR(device_create(my_class, NULL, dev, NULL, "trivdev"))) {
        printk(KERN_ERR "trivmod: failed to create device\n");
        goto err_class;
    }
    printk(KERN_INFO "trivmod: /dev/trivdev created\n");

    printk(KERN_INFO "hello world\n");
    return 0;

err_class:
    class_destroy(my_class);
err_cdev:
    cdev_del(&my_dev.cdev);
err_region:
    unregister_chrdev_region(dev, 1);
    return -1;
}

static void __exit mymod_exit(void)
{
    printk(KERN_INFO "trivmod: final stats: opens=%d closes=%d\n",
           my_dev.open_count, my_dev.release_count);

    device_destroy(my_class, dev);
    class_destroy(my_class);
    cdev_del(&my_dev.cdev);
    unregister_chrdev_region(dev, 1);
    printk(KERN_INFO "bye bye world\n");
}

module_init(mymod_init);
module_exit(mymod_exit);
