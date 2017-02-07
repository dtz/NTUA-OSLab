/*
 * lunix-chrdev.c
 *
 * Implementation of character devices
 * for Lunix:TNG
 *
 * Dimitris Sarlis 
 * Dimitris Tzannetos 
 *
 */

#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mmzone.h>
#include <linux/vmalloc.h>
#include <linux/spinlock.h>

#include "lunix.h"
#include "lunix-chrdev.h"
#include "lunix-lookup.h"

/*
 * Global data
 */
struct cdev lunix_chrdev_cdev;

/*
 * Just a quick [unlocked] check to see if the cached
 * chrdev state needs to be updated from sensor measurements.
 */
static int lunix_chrdev_state_needs_refresh(struct lunix_chrdev_state_struct *state)
{
	struct lunix_sensor_struct *sensor;
	
	WARN_ON ( !(sensor = state->sensor));

	debug("state needs refreshing!\n");
	return (sensor->msr_data[state->type]->last_update != state->buf_timestamp);
 
}

/*
 * Updates the cached state of a character device
 * based on sensor data. Must be called with the
 * character device state lock held.
 */
static int lunix_chrdev_state_update(struct lunix_chrdev_state_struct *state)
{
	struct lunix_sensor_struct *sensor;
	uint16_t value;
	uint32_t curr_timestamp;
	unsigned long flags;
	int ret;
	long res ; 
	debug("entering\n");

	sensor = state->sensor;
	ret = -EAGAIN;
    	spin_lock_irqsave(&sensor->lock, flags);
	value = sensor->msr_data[state->type]->values[0];
	curr_timestamp = sensor->msr_data[state->type]->last_update;
	spin_unlock_irqrestore(&sensor->lock, flags);

	/*
	 * Grab the raw data quickly, hold the
	 * spinlock for as little as possible.
	 */

	/*
	 * Any new data available?
	 */
	/*
	 * Now we can take our time to format them,
	 * holding only the private state semaphore 
	 * lunix_chrdev_open handles the private state
	 * semaphore (functions P and V ) 
	 */
	res = value;
	if ( lunix_chrdev_state_needs_refresh(state) ) {
		// new data available 
		if (state->data_type) {
			switch(state->type) {
		/* refer to  lunix-lookup.h tables generated during the make process
		 * to obtain the xxyyy formatted value of {BATT,TEMP,LIGHT} respectively
		 */
				case BATT: res = lookup_voltage[value];
				 	  break;
				case TEMP: res = lookup_temperature[value];
                                   	  break;
				case LIGHT: res = lookup_light[value];
                                   	  break;
				default : 
				   	debug("Lunix:TNG Internal Error @  sensor type switching \n");
				   	ret = -EMEDIUMTYPE ; 
				   	goto out ;
			 }
			 state->buf_lim = snprintf(state->buf_data, LUNIX_CHRDEV_BUFSZ, "%ld.%03ld\n", res/1000, res%1000);
		}	
		/* save formatted data */ 
		else state->buf_lim = snprintf(state->buf_data, LUNIX_CHRDEV_BUFSZ, "%ld\n", res);
		state->buf_timestamp = curr_timestamp;
		ret = 0;
	}
	else {
		
		// no NEW data available
		ret = -EAGAIN;
	}
out : 
	debug("leaving with ret: %d\n", ret);
	return ret;
}

/*************************************
 * Implementation of file operations
 * for the Lunix character device
 *************************************/

static int lunix_chrdev_open(struct inode *inode, struct file *filp)
{
	/* Declarations */
	unsigned int minor, sensor, type;
	struct lunix_chrdev_state_struct *state;
	int ret;

	debug("entering\n");
	ret = -ENODEV;
	if ((ret = nonseekable_open(inode, filp)) < 0)
		goto out;

	/*
	 * Associate this open file with the relevant sensor based on
	 * the minor number of the device node [/dev/sensor<NO>-<TYPE>]
	 */
	minor = iminor(inode);
	sensor = minor / 8;
	type = minor % 8;
	debug("associating file with sensor %d and type %d\n", sensor, type);
	/* Allocate a new Lunix character device private state structure */

	 state = kmalloc(sizeof(struct lunix_chrdev_state_struct), GFP_KERNEL);
	 if (!state) {
		debug("kmalloc: could not allocate requested memory\n");
		ret = -ENOMEM;
	    goto out;
	 }
	 state->type = type;
	 state->sensor = &lunix_sensors[sensor];
	 state->buf_lim = 0 ; 
	 state->buf_timestamp = 0 ; 
	 state->data_type = 1;
	 filp->private_data = state;
	 // initializing semaphore with value 1 (access to file from read) 
	 sema_init(&state->lock,1);
	 debug("chrdev state initialized successfully!\n");
	 ret = 0;
out:
	debug("leaving with ret = %d\n", ret);
	return ret;
}

static int lunix_chrdev_release(struct inode *inode, struct file *filp)
{
	debug("releasing allocated memory for private file data\n");
	kfree(filp->private_data);
	return 0;
}

static long lunix_chrdev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int retval = -ENOTTY;
	struct lunix_chrdev_state_struct *state;
	/*
 	* extract the type and number bitfields, and don't decode
 	* wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok( )
 	*/
 	if (_IOC_TYPE(cmd) != LUNIX_IOC_MAGIC) return retval;
 	if (_IOC_NR(cmd) > LUNIX_IOC_MAXNR) return retval;

	state = filp->private_data;

	switch(cmd) {
  		case LUNIX_IOC_DATA_TYPE:
			if (down_interruptible(&state->lock))
                		return -ERESTARTSYS;
    			state->data_type = (state->data_type) ? 0 : 1;
			up (&state->lock);	
    			break;
		default: /* redundant, as cmd was checked against MAXNR */
  			return -ENOTTY;
	}

	debug("successfully changed data type transfer\n");

	return 0;
}

static ssize_t lunix_chrdev_read(struct file *filp, char __user *usrbuf, size_t cnt, loff_t *f_pos)
{
	ssize_t ret,remain_bytes;
	
	struct lunix_sensor_struct *sensor;
	struct lunix_chrdev_state_struct *state;
	
	state = filp->private_data;
	WARN_ON(!state);

	sensor = state->sensor;
	WARN_ON(!sensor);
	debug("entering!\n");
	if (down_interruptible(&state->lock)) 
		return -ERESTARTSYS;
	/*
	 * If the cached character device state needs to be
	 * updated by actual sensor data (i.e. we need to report
	 * on a "fresh" measurement, do so
	 */
	if (*f_pos == 0) {
		
		while (lunix_chrdev_state_update(state) == -EAGAIN) {
			/* ? */
			/* The process needs to sleep */
			/* See LDD3, page 153 for a hint */

			up(&state->lock);
			/*if (filp->f_flags & O_NONBLOCK)
				return -EAGAIN;*/
			if (wait_event_interruptible(sensor->wq, lunix_chrdev_state_needs_refresh(state)))
				return -ERESTARTSYS;
			if (down_interruptible(&state->lock))
				return -ERESTARTSYS;			
		}
	}

	
	/* Determine the number of cached bytes to copy to userspace */
	remain_bytes = state->buf_lim - *f_pos;
	/* if the user requested more bytes than the cached_bytes
	 * just return the cached ones (used for copy_to_user)
	 */
	cnt = ( cnt < remain_bytes ) ? cnt : remain_bytes;
	// note : copy_to_user returns number of bytes that could not be copied 
	if (copy_to_user(usrbuf, (state->buf_data + *f_pos), cnt)) {
		ret = -EFAULT;
		goto out ; 
	}
	// update f_pos 
	*f_pos += cnt;
	ret = cnt;
	
	/* Auto-rewind on EOF mode? */
	if (*f_pos == state->buf_lim)
		*f_pos = 0;
out:
	/* Unlock? */
	up (&state->lock);
	return ret;
}

static int lunix_chrdev_mmap(struct file *filp, struct vm_area_struct *vma)
{
	return -EINVAL;
}

static struct file_operations lunix_chrdev_fops = 
{
    .owner          = THIS_MODULE,
	.open           = lunix_chrdev_open,
	.release        = lunix_chrdev_release,
	.read           = lunix_chrdev_read,
	.unlocked_ioctl = lunix_chrdev_ioctl,
	.mmap           = lunix_chrdev_mmap
};

int lunix_chrdev_init(void)
{
	/*
	 * Register the character device with the kernel, asking for
	 * a range of minor numbers (number of sensors * 8 measurements / sensor)
	 * beginning with LINUX_CHRDEV_MAJOR:0
	 */
	int ret;
	dev_t dev_no;
	unsigned int lunix_minor_cnt = lunix_sensor_cnt << 3;
	
	debug("initializing character device\n");
	cdev_init(&lunix_chrdev_cdev, &lunix_chrdev_fops);
	lunix_chrdev_cdev.owner = THIS_MODULE;
	
	dev_no = MKDEV(LUNIX_CHRDEV_MAJOR, 0);
	ret = register_chrdev_region(dev_no, lunix_minor_cnt, "lunix");
	if (ret < 0) {
		debug("failed to register region, ret = %d\n", ret);
		goto out;
	}	
	debug("device registered successfully!\n");
	ret = cdev_add(&lunix_chrdev_cdev, dev_no, lunix_minor_cnt);
	if (ret < 0) {
		debug("failed to add character device\n");
		goto out_with_chrdev_region;
	}
	debug("completed successfully\n");
	return 0;

out_with_chrdev_region:
	unregister_chrdev_region(dev_no, lunix_minor_cnt);
out:
	return ret;
}

void lunix_chrdev_destroy(void)
{
	dev_t dev_no;
	unsigned int lunix_minor_cnt = lunix_sensor_cnt << 3;
		
	debug("entering\n");
	dev_no = MKDEV(LUNIX_CHRDEV_MAJOR, 0);
	cdev_del(&lunix_chrdev_cdev);
	unregister_chrdev_region(dev_no, lunix_minor_cnt);
	debug("leaving\n");
}
