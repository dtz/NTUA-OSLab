/*
 * crypto-module.c
 *
 * virtio-crypto module for linux guest kernel
 *
 * Dimitris Siakavaras <jimsiak@cslab.ece.ntua.gr>
 *
 */

#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/virtio_config.h>

#include "crypto-chrdev.h"
#include "crypto-vq.h"
#include "cryptodev.h"

struct crypto_driver_data crdrvdata;
DEFINE_SPINLOCK(crdrvdata_lock);

static void handle_control_message(struct crypto_device *crdev,
                                   struct crypto_vq_buffer *buf)
{
	struct virtio_crypto_control *cpkt;

	cpkt = (struct virtio_crypto_control *)(buf->buf + buf->offset);

	switch (cpkt->event) {
	case VIRTIO_CRYPTO_DEVICE_HOST_OPEN:
		crdev->fd = cpkt->value;
		printk(KERN_WARNING "fd=%d\n",crdev->fd);
		wake_up_interruptible(&crdev->c_wq);
		break;
	default:
		return;
	}
}

/*
 * Host sent us a control message.
 * Called in interrupt context!
 */
static void control_in_intr(struct virtqueue *vq)
{
	struct crypto_device *crdev;
	struct crypto_vq_buffer *buf;
	unsigned int len;

	debug("Entering\n");
	crdev = vq->vdev->priv;

	while ((buf = virtqueue_get_buf(vq, &len))) {
		
		buf->len = len;
		buf->offset = 0;
		
		handle_control_message(crdev, buf);
		
		if (add_inbuf(crdev->c_ivq, buf) < 0) {
			printk(KERN_WARNING "Error adding buffer to queue\n");
			free_buf(buf);
		}
	}
	debug("Leaving\n");
}

static void control_out_intr(struct virtqueue *vq)
{
	debug("Entering\n");
	debug("Leaving\n");
}

/* Host sent us data. */
static void in_intr(struct virtqueue *vq)
{
	struct crypto_device *crdev;
	debug("Entering\n");
	crdev = vq->vdev->priv;

	crdev->inbuf = get_inbuf(crdev);
	wake_up_interruptible(&crdev->i_wq);
	debug("Leaving\n");
}

static void out_intr(struct virtqueue *vq)
{
	debug("Entering\n");
	debug("Leaving\n");
}

/*
 * Find the queues of the virtio crypto device.
 */
static int find_vqs(struct crypto_device *crdev)
{
	int err;
	vq_callback_t **io_callbacks;
	char **io_names;
	struct virtqueue **vqs;
	u32 nr_queues = 4;

	vqs = kmalloc(nr_queues * sizeof(struct virtqueue *), GFP_KERNEL);
	io_callbacks = kmalloc(nr_queues * sizeof(vq_callback_t *), GFP_KERNEL);
	io_names = kmalloc(nr_queues * sizeof(char *), GFP_KERNEL);
	if (!vqs || !io_callbacks || !io_names) {
		err = -ENOMEM;
		goto free;
	}

	io_names[0] = "control_in";
	io_names[1] = "control_out";
	io_names[2] = "input";
	io_names[3] = "output";

	io_callbacks[0] = control_in_intr;
	io_callbacks[1] = control_out_intr;
	io_callbacks[2] = in_intr;
	io_callbacks[3] = out_intr;
	
	err = crdev->vdev->config->find_vqs(crdev->vdev, nr_queues, vqs,
	                                    io_callbacks,
	                                    (const char **)io_names);
	if (err)
		goto free;

	crdev->c_ivq = vqs[0];
	crdev->c_ovq = vqs[1];
	crdev->ivq = vqs[2];
	crdev->ovq = vqs[3];

	crdev->c_ivq->vdev->priv = crdev;
	crdev->c_ovq->vdev->priv = crdev;
	crdev->ivq->vdev->priv = crdev;
	crdev->ovq->vdev->priv = crdev;

	return 0;

free:
	kfree(io_names);
	kfree(io_callbacks);
	kfree(vqs);
	
	return err;
}

/*
 * This function is called each time a the kernel finds a virtio device
 * that we are associated with.
 */
static int __devinit virtcons_probe(struct virtio_device *vdev)
{
	int err;
	struct crypto_device *crdev;
	unsigned int nr_added_bufs;

	debug("Entering\n");
	crdev = kzalloc(sizeof(*crdev), GFP_KERNEL);
	if (!crdev) {
		err = -ENOMEM;
		goto fail;
	}

	crdev->vdev = vdev;
	vdev->priv = crdev;

	/* Since the open from the host will return -1 we
	   initiaate this to -13 to indicate no open try yet. */
	crdev->fd = -13;

	spin_lock_irq(&crdrvdata_lock);
	crdev->minor = crdrvdata.next_minor++;
	spin_unlock_irq(&crdrvdata_lock);

	/* Find the virtqueues that the device will user. */
	err = find_vqs(crdev);
	if (err < 0) {
		printk(KERN_WARNING "Error %d initializing vqs\n", err);
		goto fail;
	}

	init_waitqueue_head(&crdev->c_wq);
	init_waitqueue_head(&crdev->i_wq);
	
	/* Initialize all locks. */
	/* Do i need any lock? */
	/* ? */

	/* Fill the in_vq with buffers so the host can send us data. */
	nr_added_bufs = fill_queue(crdev->ivq, NULL);
	if (!nr_added_bufs) {
	printk(KERN_WARNING "Error allocating inbufs\n");
		err = -ENOMEM;
		goto fail;
	}

	/* Fill the c_ivq with buffers so the host can send us data. */
	nr_added_bufs = fill_queue(crdev->c_ivq, NULL);
	if (!nr_added_bufs) {
		printk(KERN_WARNING "Error allocating buffers for control queue\n");
		err = -ENOMEM;
		goto fail;
	}

	/* Finally add the device to the driver list. */
	spin_lock_irq(&crdrvdata_lock);
	list_add_tail(&crdev->list, &crdrvdata.devs);
	spin_unlock_irq(&crdrvdata_lock);

	debug("Leaving\n");
	return 0;

fail:
	return err;
}

static void virtcons_remove(struct virtio_device *vdev)
{
	struct crypto_device *crdev;
	debug("Entering\n");
	crdev = vdev->priv;

	/* Delete virtio device list entry. */
	spin_lock_irq(&crdrvdata_lock);
	list_del(&crdev->list);
	spin_unlock_irq(&crdrvdata_lock);

	/* NEVER forget to delete device virtqueues. */
	vdev->config->del_vqs(vdev);

	kfree(crdev);
	debug("Leaving\n");
}

static struct virtio_device_id id_table[] = {
	{ VIRTIO_ID_CRYPTO, VIRTIO_DEV_ANY_ID },
	{ 0 },
};

static unsigned int features[] = {
	0
};

static struct virtio_driver virtio_crypto = {
	.feature_table = features,
	.feature_table_size = ARRAY_SIZE(features),
	.driver.name =	KBUILD_MODNAME,
	.driver.owner =	THIS_MODULE,
	.id_table =	id_table,
	.probe =	virtcons_probe,
	.remove =	virtcons_remove,
};

/*
 * The function that is called when our module is being inserted in
 * the running kernel.
 */
static int __init init(void)
{
	int ret;
	debug("Entering\n");
	/* Register the character devices that we will use. */
	ret = crypto_chrdev_init();
	if (ret < 0) {
		printk(KERN_WARNING "Could not initialize character devices.\n");
		goto out;
	}

	INIT_LIST_HEAD(&crdrvdata.devs);
	crdrvdata.next_minor = 0;

	/* Register the virtio driver. */
	ret = register_virtio_driver(&virtio_crypto);
	if (ret < 0) {
		printk(KERN_WARNING "Failed to register virtio driver.\n");
		goto out_with_chrdev;
	}

	debug("Leaving\n");
	return ret;

out_with_chrdev:
	crypto_chrdev_destroy();
out:
	return ret;
}

/*
 * The function that is called when our module is being removed.
 * Make sure to cleanup everything.
 */
static void __exit fini(void)
{
	debug("Entering\n");
	crypto_chrdev_destroy();
	unregister_virtio_driver(&virtio_crypto);
	debug("Leaving\n");
}

module_init(init);
module_exit(fini);

MODULE_DEVICE_TABLE(virtio, id_table);
MODULE_DESCRIPTION("Virtio crypto driver");
MODULE_LICENSE("GPL");
