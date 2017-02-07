#ifndef _CRYPTO_VQ_H
#define _CRYPTO_VQ_H

#include "crypto.h"

/*
 * This struct represents the buffers that are being excanged via the virtqueues.
 */
struct crypto_vq_buffer {
	char *buf;
	
	/* size of the buffer in *buf above */
	size_t size;
	
	/* used length of the buffer */
	size_t len;
	/* offset in the buf from which to consume data */
	size_t offset;
};

struct crypto_vq_buffer *get_inbuf(struct crypto_device *crdev);

ssize_t fill_readbuf(struct crypto_device *crdev, char *out_buf,
                     size_t out_count);

bool device_has_data(struct crypto_device *crdev);

ssize_t send_control_msg(struct crypto_device *crdev, unsigned int event,
                         unsigned int value);

/* Callers must take the port->outvq_lock */
void reclaim_consumed_buffers(struct crypto_device *crdev);

/*
 * Send a buffer to the output virtqueue of the given crypto_device. 
 * If nonblock is false wait util the host acknowledges the data receive.
 */
ssize_t send_buf(struct crypto_device *crdev, void *in_buf, size_t in_count,
                 bool nonblock);

void free_buf(struct crypto_vq_buffer *buf);
int add_inbuf(struct virtqueue *vq, struct crypto_vq_buffer *buf);
unsigned int fill_queue(struct virtqueue *vq, spinlock_t *lock);
#endif
