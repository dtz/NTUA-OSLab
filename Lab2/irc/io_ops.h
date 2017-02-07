#ifndef _IO_OPS_H
#define _IO_OPS_H

#include <sys/types.h>
#include <fcntl.h>

int fill_urandom_buf(unsigned char *buf, size_t cnt);
ssize_t insist_write(int fd, const void *buf, size_t cnt);
ssize_t insist_read(int fd, void *buf, size_t cnt);

#endif
