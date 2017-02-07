/*                                                                                                                                            
 * crypto-vq.c
 *
 * virtqueues operations for
 * virtio-crypto module 
 *
 * Dimitris Siakavaras <jimsiak@cslab.ece.ntua.gr>
 * Stefanos Gerangelos <sgerag@cslab.ece.ntua.gr>
 *
 */

#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/virtio.h>
#include "crypto-vq.h"
#include "crypto-chrdev.h" /* Only for debug(). */

/*
 * Get a buffer from the ivq. 
 * If we already have one return it.
 */
struct crypto_vq_buffer *get_inbuf(struct crypto_device *crdev)
{
	struct crypto_vq_buffer *buf;
	unsigned int len;
	
	debug("Entering\n");
	if (crdev->inbuf)
		return crdev->inbuf;
	
	buf = virtqueue_get_buf(crdev->ivq, &len);
	if (buf) {
		buf->len = len;
		buf->offset = 0;
	}
	debug("Leaving\n");
	return buf;
}

bool device_has_data(struct crypto_device *crdev)
{
	//unsigned long flags;
	bool ret;
	
	debug("Entering\n");
	ret = false;
	//spin_lock_irqsave(&port->inbuf_lock, flags);
	crdev->inbuf = get_inbuf(crdev);
	if (crdev->inbuf)
		ret = true;
	debug("Leaving\n");

	debug("returning with value %d\n", ret);
	
	//spin_unlock_irqrestore(&port->inbuf_lock, flags);
	return ret;
}


/*
 * Give out the data that's requested from the buffer that we have
 * queued up.
 */
ssize_t fill_readbuf(struct crypto_device *crdev, char *out_buf, 
                     size_t out_count)
{
	struct crypto_vq_buffer *buf;

	debug("Entering\n");
	if (!out_count || !device_has_data(crdev))
		return 0;

	buf = crdev->inbuf;
	out_count = min(out_count, buf->len - buf->offset);

	memcpy(out_buf, buf->buf + buf->offset, out_count);

	buf->offset += out_count;

	if (buf->offset == buf->len) {
		/*
		 * FIXME: We're done using all the data in this buffer.
		 * Re-queue so that the Host can send us more data.
		 */
		crdev->inbuf = NULL;

		if (add_inbuf(crdev->ivq, buf) < 0)
			printk(KERN_WARNING "failed add_buf\n");

	}
	debug("Leaving\n");
	/* Return the number of bytes actually copied */
	return out_count;
}

/*
 * Send a control message to the Guest. 
 */
ssize_t send_control_msg(struct crypto_device *crdev, unsigned int event,
                         unsigned int value)
{
	struct scatterlist sg[1];
	struct virtio_crypto_control cpkt;
	struct virtqueue *vq;
	unsigned int len;
	
	debug("Entering\n");
	cpkt.event = event;
	cpkt.value = value;
	
	vq = crdev->c_ovq;
	
	sg_init_one(sg, &cpkt, sizeof(cpkt));
	if (virtqueue_add_buf(vq, sg, 1, 0, &cpkt) >= 0) {
		virtqueue_kick(vq);
		/* Wait until host gets the message. */
		while (!virtqueue_get_buf(vq, &len))
			cpu_relax();
	}
	debug("Leaving\n");
	return 0;
}

/* Free consumed buffers. */
void reclaim_consumed_buffers(struct crypto_device *crdev)
{
	void *buf;
	unsigned int len;
	
	debug("Entering\n");
	while ((buf = virtqueue_get_buf(crdev->ovq, &len))) 
		kfree(buf);
	debug("Leaving\n");
}

/*
 * Send a buffer to the output virtqueue of the given crypto_device. 
 * If nonblock is false wait until the host acknowledges the data receive.
 */
ssize_t send_buf(struct crypto_device *crdev, void *in_buf, size_t in_count,
                 bool nonblock)
{
	struct scatterlist sg[1];
	struct virtqueue *out_vq;
	ssize_t ret = 0;
	unsigned int len;
	
	debug("Entering\n");
	out_vq = crdev->ovq;
	
	/* Discard any consumed buffers. */
	reclaim_consumed_buffers(crdev);
	
	sg_init_one(sg, in_buf, in_count);
	
	/* add sg list to virtqueue and notify host */
	ret = virtqueue_add_buf(out_vq, sg, 1, 0, in_buf);
	
	if (ret < 0) {
		debug("Oops! Error adding buffer to vqueue\n");
		in_count = 0;
		goto done;
	}
	
	if (ret == 0) {
		printk(KERN_WARNING "ovq_full!!!!\n");
		crdev->ovq_full = true;
	}

	virtqueue_kick(out_vq); 
	
	if (nonblock)
		goto done;
	
	/*
	 * if nonblock is false we wait until the host acknowledges it pushed 
	 * out the data we sent.
	 */
	while (!virtqueue_get_buf(out_vq, &len))
		cpu_relax();
	debug("Leaving\n");

done:
	/*
	 * We're expected to return the amount of data we wrote -- all
	 * of it
	 */
	return in_count;
}

/*
 * Free a virtqueue buffer. 
 */
void free_buf(struct crypto_vq_buffer *buf)
{
	debug("Entering\n");
	kfree(buf->buf);
	kfree(buf);
	debug("Leaving\n");
}

/*
 * Allocate a buffer. 
 */
static struct crypto_vq_buffer *alloc_buf(size_t buf_size)
{
	struct crypto_vq_buffer *buf;
	
	debug("Entering\n");
	buf = kmalloc(sizeof(*buf), GFP_KERNEL);
	if (!buf)
		goto fail;
	buf->buf = kzalloc(buf_size, GFP_KERNEL);
	if (!buf->buf)
		goto free_buf;
	buf->len = 0;
	buf->offset = 0;
	buf->size = buf_size;
	debug("Leaving\n");
	return buf;

free_buf:
	kfree(buf);
fail:
	return NULL;
}

/*
 * Add a buffer to the specified virtqueue.
 * Callers should take appropriate locks.
 */
int add_inbuf(struct virtqueue *vq, struct crypto_vq_buffer *buf)
{
	struct scatterlist sg[1];
	int ret;
	
	debug("Entering\n");
	/* Let's make the scatter-gather list. */
	sg_init_one(sg, buf->buf, buf->size);
	
	/* Ok now add buf to virtqueue and notify host. */
	ret = virtqueue_add_buf(vq, sg, 0, 1, buf);
	virtqueue_kick(vq);
	debug("Leaving\n");
	
	return ret;
}

/*
 * Fill a virtqueue with buffers so Host can send us data.
 */
unsigned int fill_queue(struct virtqueue *vq, spinlock_t *lock)
{
	struct crypto_vq_buffer *buf;
	unsigned int nr_added_bufs;
	int ret;
	
	debug("Entering\n");
	nr_added_bufs = 0;
	do {
		buf = alloc_buf(PAGE_SIZE);
		if (!buf)
		break;
		
		ret = add_inbuf(vq, buf);
		if (ret < 0) {
			free_buf(buf);
			break;
		}
		nr_added_bufs++;
	} while (ret > 0);
	debug("Leaving\n");
	
	return nr_added_bufs;
}
