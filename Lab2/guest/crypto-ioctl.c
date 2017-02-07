/*
 * crypto-ioctl.c
 *
 * Implementation of ioctl for 
 * virtio-crypto (guest side) 
 *
 * Stefanos Gerangelos <sgerag@cslab.ece.ntua.gr>
 *                                                                               
 */

#include <linux/cdev.h>
#include <linux/completion.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/freezer.h>
#include <linux/list.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/virtio.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/module.h>
#include <linux/ioctl.h>

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include "crypto.h"
#include "crypto-vq.h"
#include "crypto-chrdev.h"

long crypto_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct crypto_device *crdev;
	struct crypto_data *cr_data;
	struct session_op *usr_sess_data;
	struct crypt_op *usr_crypt_data;
	ssize_t ret;

	crdev = filp->private_data;

	cr_data = kzalloc(sizeof(crypto_data), GFP_KERNEL);
	if (!cr_data)
		return -ENOMEM;

	cr_data->cmd = cmd;
	
	debug("In our ioctl\n");

	switch (cmd) {

        /* get the metadata for every operation 
	 * from userspace and send the buffer 
         * to the host */

	case CIOCGSESSION:
		usr_sess_data = (struct session_op *)arg;
		ret = copy_from_user(&cr_data->op.sess, usr_sess_data, sizeof(struct session_op));
		if (ret) {
			ret = -EFAULT;
			goto free_buf;
		}


		memcpy(cr_data->keyp , cr_data->op.sess.key, cr_data->op.sess.keylen);
		cr_data->success = 1;
		debug("cipher: %d, keylen: %d, key: %s\n", cr_data->op.sess.cipher, cr_data->op.sess.keylen, cr_data->op.sess.key);
		

		ret = send_buf(crdev, cr_data, sizeof(struct crypto_data), 0);
		if (ret < 0)
			goto free_buf;

		if (!device_has_data(crdev)) {
			debug("sleeping in CIOCGSESSION\n");
			if (filp->f_flags & O_NONBLOCK)
				return -EAGAIN;

			/* Go to sleep unti we have data. */
			ret = wait_event_interruptible(crdev->i_wq,
			                               device_has_data(crdev));

			if (ret < 0) 
				goto free_buf;
		}
		printk(KERN_ALERT "woke up in CIOCGSESSION\n");

		if (!device_has_data(crdev))
			return -ENOTTY;

		ret = fill_readbuf(crdev, (char *)cr_data, sizeof(crypto_data));
		debug("sess_id=%d\n", cr_data->op.sess.ses);
		/* copy the response to userspace */
		
		if (!cr_data->success) {
			ret = -EINVAL;
			goto free_buf;
		}

		cr_data->op.sess.key = usr_sess_data->key;
		memcpy(cr_data->op.sess.key, cr_data->keyp, cr_data->op.sess.keylen);
		ret = copy_to_user(usr_sess_data, &cr_data->op.sess, sizeof(struct session_op));
		if (ret) {
			ret = -EFAULT;
			goto free_buf;
		}

		debug("returning with value=%d\n", (int)ret);

		break;

	case CIOCCRYPT: 
		debug("in encryption/decryption\n");
		usr_crypt_data = (struct crypt_op *)arg;
		ret = copy_from_user(&cr_data->op.crypt, usr_crypt_data, sizeof(struct crypt_op));
		if (ret) {
			ret = -EFAULT;
			goto free_buf;
		}


		cr_data->success = 1;

		debug("after copy_from_user\n");

		memcpy(cr_data->srcp, cr_data->op.crypt.src, cr_data->op.crypt.len);
		memcpy(cr_data->dstp, cr_data->op.crypt.dst, cr_data->op.crypt.len);
		memcpy(cr_data->ivp, cr_data->op.crypt.iv, sizeof(cr_data->op.crypt.iv));
                
		ret = send_buf(crdev, cr_data, sizeof(struct crypto_data), 0); 
		if (ret < 0)
                        goto free_buf;

		debug("encrypt/decrypt: successfully sent data\n");

		if (!device_has_data(crdev)) {
			printk(KERN_WARNING "sleeping in CIOCCRYPTO\n");	
			if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
			
			/* Go to sleep until we have data. */
			ret = wait_event_interruptible(crdev->i_wq,
			device_has_data(crdev));
			
			if (ret < 0)
			goto free_buf;
		}

		ret = fill_readbuf(crdev, (char *)cr_data, sizeof(crypto_data));
		debug("successfully read data from buffer\n");

		if (!cr_data->success) {
			ret = -EINVAL;
			goto free_buf;
		}		

		cr_data->op.crypt.src = usr_crypt_data->src;
		cr_data->op.crypt.dst = usr_crypt_data->dst;
		cr_data->op.crypt.iv = usr_crypt_data->iv;
		memcpy(cr_data->op.crypt.src, cr_data->srcp, cr_data->op.crypt.len);
                memcpy(cr_data->op.crypt.dst, cr_data->dstp, cr_data->op.crypt.len);
                memcpy(cr_data->op.crypt.iv, cr_data->ivp, sizeof(cr_data->op.crypt.iv));

		ret = copy_to_user(usr_crypt_data, &cr_data->op.crypt, 
		                   sizeof(struct crypt_op));
		
		if (ret) {
			ret = -EFAULT;
			goto free_buf;
		}


		break;

	case CIOCFSESSION:
		ret = copy_from_user(&cr_data->op.sess_id, (void __user*) arg, sizeof(uint32_t));
		if (ret) {
			ret = -EFAULT;
			goto free_buf;
		}

                debug("sess_id=%d\n", cr_data->op.sess_id);

		cr_data->success = 1;

                ret  = send_buf(crdev, cr_data, sizeof(struct crypto_data), 0);
		if (ret < 0)
			goto free_buf;

		if (!device_has_data(crdev)) {
			printk(KERN_WARNING "PORT HAS NOT DATA!!!\n");
			if (filp->f_flags & O_NONBLOCK)
				return -EAGAIN;

			/* Go to sleep until we have data. */
			ret = wait_event_interruptible(crdev->i_wq,
			                               device_has_data(crdev));

			if (ret < 0)
				goto free_buf;
		}

		ret = fill_readbuf(crdev, (char *)cr_data, sizeof(crypto_data));

		if (!cr_data->success) {
			ret = -EINVAL;
			goto free_buf;
		}
		/* copy the response to userspace */
                ret = copy_to_user((void __user*)arg, &cr_data->op.sess_id, sizeof(uint32_t));
                if (ret) {
                        ret = -EFAULT;
                        goto free_buf;
                }

		break;

	default:
		return -EINVAL;
	}

free_buf:
	kfree(cr_data);
	return ret;	
}
