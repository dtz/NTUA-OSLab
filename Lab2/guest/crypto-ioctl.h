#ifndef _CRYPTO_IOCTL
#define _CRYPTO_IOCTL

long crypto_ioctl(struct file *filp, unsigned int cmd, 
                  unsigned long arg);

#endif
