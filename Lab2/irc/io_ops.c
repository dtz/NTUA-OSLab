#include <unistd.h>
#include <stdio.h>

#include "io_ops.h"


int fill_urandom_buf(unsigned char *buf, size_t cnt)
{
        int crypto_fd;
        int ret = -1;
        crypto_fd = open("ivector.txt", O_RDONLY);
        if (crypto_fd < 0){
                perror("ivector");
                return crypto_fd;
        }
        ret = insist_read(crypto_fd, buf, cnt);
        close(crypto_fd);

       return ret;
}

ssize_t insist_write(int fd, const void *buf, size_t cnt)
{
	ssize_t ret;
	size_t orig_cnt = cnt;
	
	while (cnt > 0) {
	        ret = write(fd, buf, cnt);
	        if (ret < 0)
	                return ret;
	        buf += ret;
	        cnt -= ret;
	}

	return orig_cnt;
}


ssize_t insist_read(int fd, void *buf, size_t cnt)
{
        ssize_t ret;
        size_t orig_cnt = cnt;

        while (cnt > 0) {
                ret = read(fd, buf, cnt);
                if (ret < 0)
                        return ret;
		if (ret == 0)
			return (orig_cnt - cnt);
                buf += ret;
                cnt -= ret;
        }

        return orig_cnt;
}
