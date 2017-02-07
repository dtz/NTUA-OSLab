#ifndef _CRYPTO_H
#define _CRYPTO_H

#include "cryptodev.h"

/* The Virtio ID for virtio crypto ports */
#define VIRTIO_ID_CRYPTO          13

#define VIRTIO_CRYPTO_BAD_ID      (~(u32)0)

struct virtio_crypto_config {
	__u32 unused;
} __attribute__((packed));

/*
 * A message that's passed between the Host and the Guest.
 */
struct virtio_crypto_control {
	__u16 event;            /* The kind of control event (see below) */
	short value;            /* Extra information for the key */
};

/* Some events for control messages */
#define VIRTIO_CRYPTO_DEVICE_GUEST_OPEN 0
#define VIRTIO_CRYPTO_DEVICE_HOST_OPEN  1

/*
 * This is the struct that holds the global driver data.
 */
struct crypto_driver_data {
	/* The list of the devices we are handling. */
	struct list_head devs;

	/* The minor number that we give to the next device. */
	unsigned int next_minor;
};
extern struct crypto_driver_data crdrvdata;
extern spinlock_t crdrvdata_lock;


struct crypto_vq_buffer;
/*
 * This is the struct that holds per device info.
 */
struct crypto_device {
	/* Next crypto device in the list, head is in the crdrvdata struct */
	struct list_head list;

	/* VirtQueues for Host to Guest and Guest to Host control messages. */
	struct virtqueue *c_ivq, *c_ovq;

	/* VirtQueues for Host to Guest and Guest to Host communications. */
	struct virtqueue *ivq, *ovq;

	/* The virtio device we are associated with. */
	struct virtio_device *vdev;

	/* The minor number of the device. */
	unsigned int minor;

	/* The fd that this device has on the Host. */
	int fd;

	/* waitqueues for host control and input events. */
	wait_queue_head_t c_wq, i_wq;

	/* Indicates that the ovq is full. */
	bool ovq_full;

	/* The buffer that we last read from ivq. */
	struct crypto_vq_buffer *inbuf;

	/* FIXME: Do we need any lock? */
	/* ? */
};

/* This struct represents the data that we send for the ioctl(). */
typedef struct crypto_data {
	union
	{
		struct session_op sess;
		struct crypt_op crypt;
		uint32_t sess_id;
	} op;
	uint8_t keyp[CRYPTO_CIPHER_MAX_KEY_LEN];
	uint8_t srcp[CRYPTO_DATA_MAX_LEN];
	uint8_t dstp[CRYPTO_DATA_MAX_LEN];
	uint8_t ivp[CRYPTO_BLOCK_MAX_LEN];
	unsigned int cmd;
	unsigned int success;
} crypto_data;

#endif
