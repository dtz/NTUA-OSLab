/*                                                                       
 * test_crypto.c
 * 
 * Performs a simple encryption-decryption 
 * of urandom data from /dev/urandom with the 
 * use of cryptodev device.
 *
 * Stefanos Gerangelos <sgerag@cslab.ece.ntua.gr>
 *                                                                               
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include "cryptodev.h"
 
#include <sys/types.h>
#include <sys/stat.h>

#define DATA_SIZE       256
#define BLOCK_SIZE      16
#define KEY_SIZE        24

int fill_urandom_buff(char in[DATA_SIZE], int len);

static int test_crypto(int cfd)
{
	int i = -1;
        struct session_op sess;
        struct crypt_op cryp;
        struct {
                char    in[DATA_SIZE],
                        encrypted[DATA_SIZE],
                        decrypted[DATA_SIZE],
                        iv[BLOCK_SIZE],
                        key[KEY_SIZE];
        } data;

        memset(&sess, 0, sizeof(sess));
        memset(&cryp, 0, sizeof(cryp));

	if (fill_urandom_buff(data.in, DATA_SIZE) < 0) {		
		printf("error @filling urandom data\n");
		return 1;
	}

	if (fill_urandom_buff(data.key, KEY_SIZE) < 0) {
		printf("error @filling urandom data\n");
                return 1;
	}

	if (fill_urandom_buff(data.iv, BLOCK_SIZE) < 0) {
                printf("error @filling urandom data\n");
                return 1;
        }

	printf("\ninput data:");
	for (i = 0; i < DATA_SIZE; i++) {
		printf("%x", data.in[i]);
	}
	printf("\n");

	//printf("Sleeping for 30 seconds. Its time to remove the host device.\n");
	//sleep(30);
	//printf("Woke up after 30 seconds.\n");

        /* Get crypto session for AES128 */
        sess.cipher = CRYPTO_AES_CBC;
        sess.keylen = KEY_SIZE;
        sess.key = (__u8  __user *)data.key;

        if (ioctl(cfd, CIOCGSESSION, &sess)) {
                perror("ioctl(CIOCGSESSION)");
                return 1;
        }

        /* Encrypt data.in to data.encrypted */
        cryp.ses = sess.ses;
        cryp.len = sizeof(data.in);
        cryp.src = (__u8 __user *)data.in;
        cryp.dst = (__u8 __user *)data.encrypted;
        cryp.iv = (__u8 __user *)data.iv;
        cryp.op = COP_ENCRYPT;
	printf("\nencrypted data:");
        if (ioctl(cfd, CIOCCRYPT, &cryp)) {
                perror("ioctl(CIOCCRYPT)");
                return 1;
        }

	for (i = 0; i < DATA_SIZE; i++) {
		printf("%x", data.encrypted[i]);
	}
	printf("\n");
	
        /* Decrypt data.encrypted to data.decrypted */
        cryp.src = (__u8 __user *)data.encrypted;
        cryp.dst = (__u8 __user *)data.decrypted;
        cryp.op = COP_DECRYPT;
        if (ioctl(cfd, CIOCCRYPT, &cryp)) {
                perror("ioctl(CIOCCRYPT)");
                return 1;
        }

	printf("\ndecrypted data:");
	for (i = 0; i < DATA_SIZE; i++) {
		printf("%x", data.decrypted[i]);
	}
	printf("\n");

        /* Verify the result */
        if (memcmp(data.in, data.decrypted, sizeof(data.in)) != 0) {
                fprintf(stderr,
                        "\nFAIL: Decrypted data are different from the input data.\n");
                return 1;
        } else
                printf("\nTest passed\n");

        /* Finish crypto session */
        if (ioctl(cfd, CIOCFSESSION, &sess.ses)) {
                perror("ioctl(CIOCFSESSION)");
                return 1;
        }

        return 0;
}

int fill_urandom_buff(char in[DATA_SIZE], int len){
	int crypto_fd = open("/dev/urandom", O_RDONLY);
	int ret = -1;
	
	if (crypto_fd < 0) 	return crypto_fd;

	ret = read(crypto_fd, (void *)in, len);

	close(crypto_fd);

	return ret;
}

int main(int argc, char **argv)
{
        int fd = -1;
		char *filename;
		char error_str[100];

		filename = (argv[1] == NULL) ? "/dev/crypto" : argv[1];
        fd = open(filename, O_RDWR, 0);
        if (fd < 0) {
                sprintf(error_str, "open %s", filename);
                perror(error_str);
                return 1;
        }

        if (test_crypto(fd)){
                return 1;
	}

        if (close(fd)) {
                perror("close(fd)");
                return 1;
        }

        return 0;
}

