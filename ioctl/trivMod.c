#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/uaccess.h>
#include "trivmod_ioctl.h"

#define BUF_SIZE 4096

static int nparam = 5;
module_param(nparam, int, 0644);

dev_t dev;
int units = 2;

static struct mod_priv {
  struct proc_dir_entry *mod_proc;
  struct cdev mod_cdev;
  struct class *mod_cl;
  struct device *mod_dev;
  int open_count;          /* statistics: how many times opened */
  int release_count;       /* statistics: how many times closed */
  int users;               /* statistics: open right now */
  int bytes_written;       /* statistics: total bytes written */
  int bytes_read;          /* statistics: total bytes read */
  char buf[BUF_SIZE];      /* the device memory: a circular buffer */
  int head;                /* next position to write */
  int tail;                /* next position to read */
} mod_priv;

/* how many bytes are waiting in the buffer */
static int buf_used (struct mod_priv *priv)
{
  return (priv->head - priv->tail + BUF_SIZE) % BUF_SIZE;
}

static int mod_open (struct inode *inode, struct file *file)
{
  struct mod_priv *priv;

  priv = container_of (inode->i_cdev, struct mod_priv, mod_cdev);
  file->private_data = priv;

  priv->open_count++;
  priv->users++;
  printk (KERN_INFO "triv_mod: open, opens=%d users=%d\n",
          priv->open_count, priv->users);
  return 0;
}

static int mod_release (struct inode *inode, struct file *file)
{
  struct mod_priv *priv = file->private_data;

  priv->release_count++;
  priv->users--;
  printk (KERN_INFO "triv_mod: release, closes=%d users=%d\n",
          priv->release_count, priv->users);
  return 0;
}

static ssize_t mod_read (struct file *file, char __user *ubuf,
                         size_t count, loff_t *ppos)
{
  struct mod_priv *priv = file->private_data;
  int used, n, chunk, done = 0;

  used = buf_used (priv);
  if (used == 0)
    return 0;

  n = (count < (size_t)used) ? count : used;

  while (done < n) {
    chunk = n - done;
    if (chunk > BUF_SIZE - priv->tail)
      chunk = BUF_SIZE - priv->tail;

    if (copy_to_user (ubuf + done, priv->buf + priv->tail, chunk))
      return -EFAULT;

    priv->tail = (priv->tail + chunk) % BUF_SIZE;
    done += chunk;
  }

  *ppos += done;
  priv->bytes_read += done;
  printk (KERN_INFO "triv_mod: read %d bytes, used=%d\n", done, buf_used (priv));
  return done;
}

static ssize_t mod_write (struct file *file, const char __user *ubuf,
                          size_t count, loff_t *ppos)
{
  struct mod_priv *priv = file->private_data;
  int space, n, chunk, done = 0;

  space = BUF_SIZE - 1 - buf_used (priv);
  if (space == 0)
    return -ENOSPC;

  n = (count < (size_t)space) ? count : space;

  while (done < n) {
    chunk = n - done;
    if (chunk > BUF_SIZE - priv->head)
      chunk = BUF_SIZE - priv->head;

    if (copy_from_user (priv->buf + priv->head, ubuf + done, chunk))
      return -EFAULT;

    priv->head = (priv->head + chunk) % BUF_SIZE;
    done += chunk;
  }

  *ppos += done;
  priv->bytes_written += done;
  printk (KERN_INFO "triv_mod: wrote %d bytes, used=%d\n", done, buf_used (priv));
  return done;
}

static long mod_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{
  struct mod_priv *priv = file->private_data;
  struct trivmod_stats stats;

  switch (cmd) {

  case TRIVMOD_GET_STATS:
    printk (KERN_INFO "triv_mod: ioctl GET_STATS\n");
    stats.open_count    = priv->open_count;
    stats.release_count = priv->release_count;
    stats.users         = priv->users;
    stats.bytes_written = priv->bytes_written;
    stats.bytes_read    = priv->bytes_read;
    stats.buf_used      = buf_used (priv);
    if (copy_to_user ((void __user *)arg, &stats, sizeof(stats)))
      return -EFAULT;
    return 0;

  case TRIVMOD_CLEAR_STATS:
    printk (KERN_INFO "triv_mod: ioctl CLEAR_STATS\n");
    priv->open_count    = 0;
    priv->release_count = 0;
    priv->bytes_written = 0;
    priv->bytes_read    = 0;
    /* users is not cleared: it is the current state, not a history count */
    return 0;

  default:
    printk (KERN_INFO "triv_mod: ioctl unknown command 0x%x\n", cmd);
    return -ENOTTY;
  }
}

struct file_operations fops = {
  .owner          = THIS_MODULE,
  .open           = mod_open,
  .release        = mod_release,
  .read           = mod_read,
  .write          = mod_write,
  .unlocked_ioctl = mod_ioctl,
};

static int __init trivmod_init (void)
{
  int err;

  printk (KERN_INFO "triv_mod loaded, nparam = %d\n", nparam);
  mod_priv.head = 0;
  mod_priv.tail = 0;

  err = alloc_chrdev_region (&dev, 0, units, "CourseDev");
  if (err < 0)
    goto dvr_err;

  cdev_init (&mod_priv.mod_cdev, &fops);
  err = cdev_add (&mod_priv.mod_cdev, dev, units);
  if (err < 0)
    goto cdev_err;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0))
  mod_priv.mod_cl = class_create ("CourseDev");
#else
  mod_priv.mod_cl = class_create (THIS_MODULE, "CourseDev");
#endif
  if (IS_ERR(mod_priv.mod_cl)) {
    err = PTR_ERR(mod_priv.mod_cl);
    goto clc_err;
  }

  mod_priv.mod_dev = device_create (mod_priv.mod_cl, NULL, dev, NULL, "myDev");
  if (IS_ERR(mod_priv.mod_dev)) {
    err = PTR_ERR(mod_priv.mod_dev);
    goto dvc_err;
  }
  printk (KERN_INFO "triv_mod: /dev/myDev created, major %d\n", MAJOR(dev));

  return 0;
dvc_err:
  class_destroy (mod_priv.mod_cl);
clc_err:
  cdev_del (&mod_priv.mod_cdev);
cdev_err:
  unregister_chrdev_region (dev, units);
dvr_err:
  return err;
}

static void __exit trivmod_exit (void)
{
  device_destroy (mod_priv.mod_cl, dev);
  class_destroy (mod_priv.mod_cl);
  cdev_del (&mod_priv.mod_cdev);
  unregister_chrdev_region (dev, units);
  printk (KERN_INFO "triv_mod unloaded\n");
}

MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
MODULE_AUTHOR("dororiya");
MODULE_PARM_DESC(nparam, "A numeric demonstration parameter");
MODULE_DESCRIPTION("A trivial exercise module");

module_init(trivmod_init);
module_exit(trivmod_exit);
