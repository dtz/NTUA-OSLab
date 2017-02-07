/*                                                                                                                                            
 * virtio-crypto.h
 *
 * Definition file for 
 * virtio-crypto (QEMU)
 *
 * Dimitris Siakavaras <jimsiak@cslab.ece.ntua.gr>
 * Stefanos Gerangelos <sgerag@cslab.ece.ntua.gr>
 * Vangelis Koukis <vkoukis@cslab.ece.ntua.gr>
 *
 */

#ifndef _VIRTIO_CRYPTO_H
#define _VIRTIO_CRYPTO_H

#include "cryptodev.h"

/* defines for debuging reasons. */
#define FUNC_IN printf("%s: Entering\n", __func__)
#define FUNC_OUT printf("%s: Leaving\n", __func__)

#define VIRTIO_ID_CRYPTO	13

struct virtio_crypto_config {
	uint32_t test;
};
typedef struct virtio_crypto_config virtio_crypto_config;

/*
 * This struct represente the control messages that are being exchanged
 * between guest and host.
 */
struct virtio_crypto_control {
	uint16_t event;     /* The kind of control event (see below) */
	int16_t value;     /* Extra information for the key */
};

/* Some events for the internal messages (control packets) */
#define VIRTIO_CRYPTO_DEVICE_GUEST_OPEN 0
#define VIRTIO_CRYPTO_DEVICE_HOST_OPEN 	1

typedef struct VirtIOCrypto VirtIOCrypto;

VirtIODevice *virtio_crypto_init(DeviceState *dev);
void virtio_crypto_exit(VirtIODevice *vdev);

/*
 * This is the buffer that is being exchanged via the VirtQueues.
 * ATTENTION: Never use pointers or structs that contain pointers.
 */
struct virtio_crypto_buffer {
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
};
typedef struct virtio_crypto_buffer virtio_crypto_buffer;

#endif
