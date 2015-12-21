#include <sys/errno.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <fcntl.h>
#include "virtio.h"

/* FIXME: should be somewhere else */
#define LKL_DEV_BLK_TYPE_READ		0
#define LKL_DEV_BLK_TYPE_WRITE		1
#define LKL_DEV_BLK_TYPE_FLUSH		4
#define LKL_DEV_BLK_TYPE_FLUSH_OUT	5

#define LKL_DEV_BLK_STATUS_OK		0
#define LKL_DEV_BLK_STATUS_IOERR	1
#define LKL_DEV_BLK_STATUS_UNSUP	2

union lkl_disk_backstore {
	int fd;
	void *handle;
};

struct lkl_dev_blk_ops {
	int (*get_capacity)(union lkl_disk_backstore bs,
			    unsigned long long *res);
	void (*request)(union lkl_disk_backstore bs, unsigned int type,
			unsigned int prio, unsigned long long sector,
			struct lkl_dev_buf *bufs, int count);
};
struct lkl_dev_blk_ops lkl_dev_blk_ops;


/* FIXME: end */

struct virtio_blk_dev {
	struct virtio_dev dev;
	struct {
		uint64_t capacity;
	} config;
	struct lkl_dev_blk_ops *ops;
	union lkl_disk_backstore backstore;
};

struct virtio_blk_req_header {
	uint32_t type;
	uint32_t prio;
	uint64_t sector;
};

struct virtio_blk_req_trailer {
	uint8_t status;
};

static int blk_check_features(uint32_t features)
{
	if (!features)
		return 0;

	return EINVAL;
}

void lkl_dev_blk_complete(struct lkl_dev_buf *bufs, unsigned char status,
			  int len)
{
	struct virtio_dev_req *req;
	struct virtio_blk_req_trailer *f;

	req = container_of(bufs - 1, struct virtio_dev_req, buf);

	if (req->buf_count < 2) {
		printf("virtio_blk: no status buf\n");
		return;
	}

	if (req->buf[req->buf_count - 1].len != sizeof(*f)) {
		printf("virtio_blk: bad status buf\n");
	} else {
		f = req->buf[req->buf_count - 1].addr;
		f->status = status;
	}

	virtio_dev_complete(req, len);
}

static void blk_queue(struct virtio_dev *dev, struct virtio_dev_req *req)
{
	struct virtio_blk_req_header *h;
	struct virtio_blk_dev *blk_dev;

	if (req->buf[0].len != sizeof(struct virtio_blk_req_header)) {
		printf("virtio_blk: bad header buf\n");
		lkl_dev_blk_complete(&req->buf[1], LKL_DEV_BLK_STATUS_UNSUP, 0);
		return;
	}

	h = req->buf[0].addr;
	blk_dev = container_of(dev, struct virtio_blk_dev, dev);

	blk_dev->ops->request(blk_dev->backstore, le32toh(h->type),
			      le32toh(h->prio), le32toh(h->sector),
			      &req->buf[1], req->buf_count - 2);
}

static struct virtio_dev_ops blk_ops = {
	.check_features = blk_check_features,
	.queue = blk_queue,
};

/* FIXME */
static __inline long __syscall2(long n, long a1, long a2)
{
	unsigned long ret;
#ifdef __x86_64__
	__asm__ __volatile__ ("syscall" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2)
						  : "rcx", "r11", "memory");
#endif
	return ret;
}

static __inline long __syscall3(long n, long a1, long a2, long a3)
{
	unsigned long ret;
#ifdef __x86_64__
	__asm__ __volatile__ ("syscall" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2),
						  "d"(a3) : "rcx", "r11", "memory");
#endif
	return ret;
}

#define SYS_open				2
#define SYS_lseek				8

#define __sys_open2(x,pn,fl) __syscall2(SYS_open, pn, (fl))
static off_t lseek(int fd, off_t offset, int whence)
{
#ifdef SYS__llseek
	off_t result;
	return syscall(SYS__llseek, fd, offset>>32, offset, &result, whence) ? -1 : result;
#else
	return __syscall3(SYS_lseek, fd, offset, whence);
#endif
}


int lkl_disk_add(void)
{
	struct virtio_blk_dev *dev;
	unsigned long long capacity;
	int ret;
	static int count;
	union lkl_disk_backstore backstore;

	backstore.fd = __sys_open2(0, "disk.img", O_RDWR);

	dev = malloc(sizeof(*dev));
	if (!dev)
		return ENOMEM;

	dev->dev.device_id = 2;
	dev->dev.vendor_id = 0;
	dev->dev.device_features = 0;
	dev->dev.config_gen = 0;
	dev->dev.config_data = &dev->config;
	dev->dev.config_len = sizeof(dev->config);
	dev->dev.ops = &blk_ops;
	dev->ops = &lkl_dev_blk_ops;
	dev->backstore = backstore;

	ret = dev->ops->get_capacity(backstore, &capacity);
	if (ret) {
		ret = ENOMEM;
		goto out_free;
	}
	dev->config.capacity = capacity;

	ret = virtio_dev_setup(&dev->dev, 1, 65536);
	if (ret)
		goto out_free;

	return count++;

out_free:
	free(dev);

	return ret;
}

/* FIXME: from musl/ */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
/* FIXME: from posix-host.c */
int fd_get_capacity(union lkl_disk_backstore bs, unsigned long long *res)
{
	off_t off;

	off = lseek(bs.fd, 0, SEEK_END);
	if (off < 0)
		return -1;

	*res = off;
	return 0;
}

void fd_do_rw(union lkl_disk_backstore bs, unsigned int type, unsigned int prio,
	      unsigned long long sector, struct lkl_dev_buf *bufs, int count)
{
	int err = 0;
	struct iovec *iovec = (struct iovec *)bufs;

	if (count > 1)
		printf("%s: %d\n", __func__, count);

	/* TODO: handle short reads/writes */
	switch (type) {
	case LKL_DEV_BLK_TYPE_READ:
		err = preadv(bs.fd, iovec, count, sector * 512);
		break;
	case LKL_DEV_BLK_TYPE_WRITE:
		err = pwritev(bs.fd, iovec, count, sector * 512);
		break;
	case LKL_DEV_BLK_TYPE_FLUSH:
	case LKL_DEV_BLK_TYPE_FLUSH_OUT:
#ifdef __linux__
		err = fdatasync(bs.fd);
#else
		err = fsync(bs.fd);
#endif
		break;
	default:
		lkl_dev_blk_complete(bufs, LKL_DEV_BLK_STATUS_UNSUP, 0);
		return;
	}

	if (err < 0)
		lkl_dev_blk_complete(bufs, LKL_DEV_BLK_STATUS_IOERR, 0);
	else
		lkl_dev_blk_complete(bufs, LKL_DEV_BLK_STATUS_OK, err);
}

struct lkl_dev_blk_ops lkl_dev_blk_ops = {
	.get_capacity = fd_get_capacity,
	.request = fd_do_rw,
};
