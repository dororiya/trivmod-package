#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/uaccess.h>

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

  printk (KERN_INFO "triv_mod: open called (minor %d)\n", iminor(inode));

  priv = container_of (inode->i_cdev, struct mod_priv, mod_cdev);
  file->private_data = priv;

  priv->open_count++;
  priv->users++;
  printk (KERN_INFO "triv_mod: stats: opens=%d closes=%d users=%d\n",
          priv->open_count, priv->release_count, priv->users);

  return 0;
}

static int mod_release (struct inode *inode, struct file *file)
{
  struct mod_priv *priv = file->private_data;

  printk (KERN_INFO "triv_mod: release called\n");

  priv->release_count++;
  priv->users--;
  printk (KERN_INFO "triv_mod: stats: opens=%d closes=%d users=%d\n",
          priv->open_count, priv->release_count, priv->users);

  return 0;
}

static ssize_t mod_read (struct file *file, char __user *ubuf,
                         size_t count, loff_t *ppos)
{
  struct mod_priv *priv = file->private_data;
  int used, n, chunk, done = 0;

  used = buf_used (priv);
  printk (KERN_INFO "triv_mod: read called, count=%zu head=%d tail=%d used=%d\n",
          count, priv->head, priv->tail, used);

  /* head == tail: buffer is empty, return 0 = end of file */
  if (used == 0) {
    printk (KERN_INFO "triv_mod: buffer empty\n");
    return 0;
  }

  /* read only what is there */
  n = (count < (size_t)used) ? count : used;

  /* copy in pieces: from tail up to the end of the array, then wrap to 0 */
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
  printk (KERN_INFO "triv_mod: read %d bytes, head=%d tail=%d used=%d\n",
          done, priv->head, priv->tail, buf_used (priv));
  return done;
}

static ssize_t mod_write (struct file *file, const char __user *ubuf,
                          size_t count, loff_t *ppos)
{
  struct mod_priv *priv = file->private_data;
  int used, space, n, chunk, done = 0;

  used = buf_used (priv);
  space = BUF_SIZE - 1 - used;   /* one byte stays empty so full != empty */
  printk (KERN_INFO "triv_mod: write called, count=%zu head=%d tail=%d space=%d\n",
          count, priv->head, priv->tail, space);

  /* check the buffer is not full */
  if (space == 0) {
    printk (KERN_INFO "triv_mod: buffer full\n");
    return -ENOSPC;
  }

  /* write only what fits */
  n = (count < (size_t)space) ? count : space;

  /* copy in pieces: from head up to the end of the array, then wrap to 0 */
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
  printk (KERN_INFO "triv_mod: wrote %d bytes, head=%d tail=%d used=%d\n",
          done, priv->head, priv->tail, buf_used (priv));
  return done;
}

struct file_operations fops = {
  .owner   = THIS_MODULE,
  .open    = mod_open,
  .release = mod_release,
  .read    = mod_read,
  .write   = mod_write,
};

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0))
static int mod_read_proc (char *page, char **start, off_t off,
                                int count, int *eof, void *data) {
  unsigned char *content, *c;
  int len = 0;
  off_t begin = 0;

  content = (char *)(page + off);
  len = sprintf (content, "param is %d\n", nparam);
  c = (char *)content+len;
  if (off > begin + len)
    return 0;
  *start = page + (off - begin);
  *eof = 1;

  return (count < begin + len) ? count : begin + len - off;
}
#endif

static int __init trivmod_init (void)
{
  int err;

  printk (KERN_INFO "triv_mod loaded, nparam = %d, buffer = %d bytes\n",
          nparam, BUF_SIZE);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0))
  mod_priv.mod_proc = create_proc_entry ("driver/mod_proc", 0, 0);
  if (!mod_priv.mod_proc)
    return -EFAULT;
  mod_priv.mod_proc->read_proc = mod_read_proc;
#endif
  mod_priv.head = 0;
  mod_priv.tail = 0;

  err = alloc_chrdev_region (&dev, 0, units, "CourseDev");
  if (err < 0)
    goto dvr_err;
  printk (KERN_INFO "triv_mod: got major %d\n", MAJOR(dev));

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
  printk (KERN_INFO "triv_mod: /dev/myDev created\n");

  return 0;
dvc_err:
  class_destroy (mod_priv.mod_cl);
clc_err:
  cdev_del (&mod_priv.mod_cdev);
cdev_err:
  unregister_chrdev_region (dev, units);
dvr_err:
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0))
  remove_proc_entry ("driver/mod_proc", 0);
#endif

  return err;
}

static void __exit trivmod_exit (void)
{
  printk (KERN_INFO "triv_mod: final stats: opens=%d closes=%d\n",
          mod_priv.open_count, mod_priv.release_count);

  device_destroy (mod_priv.mod_cl, dev);
  class_destroy (mod_priv.mod_cl);
  cdev_del (&mod_priv.mod_cdev);
  unregister_chrdev_region (dev, units);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0))
  remove_proc_entry ("driver/mod_proc", 0);
#endif
  printk (KERN_INFO "triv_mod unloaded\n");
  return;
}

MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
MODULE_AUTHOR("dororiya");
MODULE_PARM_DESC(nparam, "A numeric demonstration parameter");
MODULE_DESCRIPTION("A trivial exercise module");

module_init(trivmod_init);
module_exit(trivmod_exit);
