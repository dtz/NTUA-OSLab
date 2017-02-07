/*
 * crypto-chrdev.c
 *
 * Implementation of character devices
 * for virtio-crypto device 
 *
 * Vangelis Koukis <vkoukis@cslab.ece.ntua.gr>
 * Dimitris Siakavaras <jimsiak@cslab.ece.ntua.gr>
 *
 */

#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/wait.h>

#include "crypto-chrdev.h"
#include "crypto-vq.h"
#include "crypto-ioctl.h"

/*
 * Global data
 */
struct cdev crypto_chrdev_cdev;

/*
 * Given the minor number of the inode return the crypto device 
 * that owns that number.
 */
static struct crypto_device *get_crypto_dev_by_minor(unsigned int minor)
{
	struct crypto_device *crdev;
	unsigned long flags;

	spin_lock_irqsave(&crdrvdata_lock, flags);
	list_for_each_entry(crdev, &crdrvdata.devs, list)
	if (crdev->minor == minor)
		goto out;
	crdev = NULL;
out:
	spin_unlock_irqrestore(&crdrvdata_lock, flags);

	return crdev;
}

/* Indicates if the real device on the Host is opened. */
static bool crypto_device_ready(struct crypto_device *crdev)
{
	bool ret = false;
	if (crdev->fd >= -1)
		ret = true;
	return ret; 
}

/*************************************
 * Implementation of file operations
 * for the Crypto character device
 *************************************/

static int crypto_chrdev_open(struct inode *inode, struct file *filp)
{
	int ret;
	ssize_t cnt;
	unsigned int minor;
	struct crypto_device *crdev;
	bool nonblock = filp->f_flags & O_NONBLOCK;

	debug("entering\n");
	ret = -ENODEV;
	if ((ret = nonseekable_open(inode, filp)) < 0)
		goto out;

	/* Associate this open file with the relevant crypto device. */
	ret = -ENODEV;
	minor = iminor(inode);
	crdev = get_crypto_dev_by_minor(minor);
	if (!crdev) {
		printk(KERN_WARNING "Could not find crypto device with %u minor\n",
		                  minor);
		goto out;
	}

	/* Only one process can open a specific device at a time. */
	/* FIXME: what about lock here? */
	if (crdev->fd >= 0) {
		debug("Too many open files. fd=%d\n", crdev->fd);
		ret = -EMFILE;
		goto out;
	}

	crdev->fd = -13;
	filp->private_data = crdev;

	/* Notify Host that we want to open the file. */
	cnt = send_control_msg(crdev, VIRTIO_CRYPTO_DEVICE_GUEST_OPEN, 1);
	

	/* Sleep here until we get the fd from the Host. */
	if (!crypto_device_ready(crdev)) {
		if (nonblock) {
			ret = -EAGAIN;
			goto out;
		}
		printk(KERN_WARNING "!crypto_device_ready(crdev)\n");
		ret = wait_event_interruptible(crdev->c_wq, 
		                               crypto_device_ready(crdev));
	}

	if (crdev->fd < 0) {
		ret = -ENODEV;
		goto out;
	}

	debug("sucessfully opened file\n");

	ret = 0;
out:
	debug("leaving, with ret = %d\n", ret);
	return ret;
}

static int crypto_chrdev_release(struct inode *inode, struct file *filp)
{
	size_t cnt;
	struct crypto_device *crdev;

	debug("Entering");
	crdev = filp->private_data;
	cnt = send_control_msg(crdev, VIRTIO_CRYPTO_DEVICE_GUEST_OPEN, 0);
	crdev->fd = -13;
	debug("Leaving");

	return 0;
}

static long crypto_chrdev_ioctl(struct file *filp, unsigned int cmd, 
                                unsigned long arg)
{
	long ret = 0;
	debug("Entering");
	ret = crypto_ioctl(filp, cmd, arg);
	debug("Leaving");

	return ret;
}

static ssize_t crypto_chrdev_read(struct file *filp, char __user *usrbuf, 
                                  size_t cnt, loff_t *f_pos)
{
	debug("Entering");
	debug("Leaving");
	return -EINVAL;
}

static struct file_operations crypto_chrdev_fops = 
{
	.owner          = THIS_MODULE,
	.open           = crypto_chrdev_open,
	.release        = crypto_chrdev_release,
	.read           = crypto_chrdev_read,
	.unlocked_ioctl = crypto_chrdev_ioctl,
};

int crypto_chrdev_init(void)
{
	/*
	 * Register the character device with the kernel, asking for
	 * a range of minor numbers (number of sensors * 8 measurements / sensor)
	 * beginning with LINUX_CHRDEV_MAJOR:0
	 */
	int ret;
	dev_t dev_no;
	unsigned int crypto_minor_cnt = CRYPTO_NR_DEVICES;
	
	debug("initializing character device\n");
	cdev_init(&crypto_chrdev_cdev, &crypto_chrdev_fops);
	crypto_chrdev_cdev.owner = THIS_MODULE;
	
	dev_no = MKDEV(CRYPTO_CHRDEV_MAJOR, 0);
	ret = register_chrdev_region(dev_no, crypto_minor_cnt, "crypto_devs");
	if (ret < 0) {
		debug("failed to register region, ret = %d\n", ret);
		goto out;
	}
	ret = cdev_add(&crypto_chrdev_cdev, dev_no, crypto_minor_cnt);
	if (ret < 0) {
		debug("failed to add character device\n");
		goto out_with_chrdev_region;
	}
	debug("completed successfully\n");
	return 0;

out_with_chrdev_region:
	unregister_chrdev_region(dev_no, crypto_minor_cnt);
out:
	return ret;
}

void crypto_chrdev_destroy(void)
{
	dev_t dev_no;
	unsigned int crypto_minor_cnt = CRYPTO_NR_DEVICES;

	debug("entering\n");
	dev_no = MKDEV(CRYPTO_CHRDEV_MAJOR, 0);
	cdev_del(&crypto_chrdev_cdev);
	unregister_chrdev_region(dev_no, crypto_minor_cnt);
	debug("leaving\n");
}
