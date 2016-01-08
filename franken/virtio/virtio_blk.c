#include <string.h>
#include <thread.h>
#include <stddef.h>
#include <errno.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <../linux/linux.h>	/* in platform/ */
#include "virtio.h"
#include "endian.h"

#include "../../../include/linux/virtio_mmio.h"
#include "../../lkl-linux/include/uapi/linux/virtio_blk.h"

/* FIXME: should be somewhere else */
#define LKL_DEV_BLK_TYPE_READ		0
#define LKL_DEV_BLK_TYPE_WRITE		1
#define LKL_DEV_BLK_TYPE_FLUSH		4
#define LKL_DEV_BLK_TYPE_FLUSH_OUT	5

#define LKL_DEV_BLK_STATUS_OK		0
#define LKL_DEV_BLK_STATUS_IOERR	1
#define LKL_DEV_BLK_STATUS_UNSUP	2

union lkl_disk {
	int fd;
	void *handle;
};

struct lkl_blk_req {
	unsigned int type;
	unsigned int prio;
	unsigned long long sector;
	struct lkl_dev_buf *buf;
	int count;
};

struct lkl_dev_blk_ops {
	int (*get_capacity)(union lkl_disk disk, unsigned long long *res);
#define LKL_DEV_BLK_STATUS_OK		0
#define LKL_DEV_BLK_STATUS_IOERR	1
#define LKL_DEV_BLK_STATUS_UNSUP	2
	int (*request)(union lkl_disk disk, struct lkl_blk_req *req);
};
struct lkl_dev_blk_ops lkl_dev_blk_ops;


/* FIXME: end */

struct virtio_blk_dev {
	struct virtio_dev dev;
	const char offset[VIRTIO_MMIO_CONFIG - sizeof(struct virtio_dev)];
	struct virtio_blk_config config;
	struct lkl_dev_blk_ops *ops;
	union lkl_disk disk;
};

struct virtio_blk_req_header {
	uint32_t type;
	uint32_t prio;
	uint64_t sector;
};

struct virtio_blk_req_trailer {
	uint8_t status;
};

static int blk_check_features(struct virtio_dev *dev)
{
	if (dev->driver_features == dev->device_features)
		return 0;

	return EINVAL;
}

static int blk_enqueue(struct virtio_dev *dev, struct virtio_req *req)
{
	struct virtio_blk_dev *blk_dev;
	struct virtio_blk_req_header *h;
	struct virtio_blk_req_trailer *t;
	struct lkl_blk_req lkl_req;

	if (req->buf_count < 3) {
		printf("virtio_blk: no status buf\n");
		goto out;
	}

	h = req->buf[0].addr;
	t = req->buf[req->buf_count - 1].addr;
	blk_dev = container_of(dev, struct virtio_blk_dev, dev);

	t->status = LKL_DEV_BLK_STATUS_IOERR;

	if (req->buf[0].len != sizeof(*h)) {
		printf("virtio_blk: bad header buf\n");
		goto out;
	}

	if (req->buf[req->buf_count - 1].len != sizeof(*t)) {
		printf("virtio_blk: bad status buf\n");
		goto out;
	}

	lkl_req.type = le32toh(h->type);
	lkl_req.prio = le32toh(h->prio);
	lkl_req.sector = le32toh(h->sector);
	lkl_req.buf = &req->buf[1];
	lkl_req.count = req->buf_count - 2;

	t->status = blk_dev->ops->request(blk_dev->disk, &lkl_req);

out:
	virtio_req_complete(req, 0);
	return 0;
}

static struct virtio_dev_ops blk_ops = {
	.check_features = blk_check_features,
	.enqueue = blk_enqueue,
};

/* FIXME */
static __inline long __syscall3(long n, long a1, long a2, long a3)
{
	unsigned long ret;
#ifdef __x86_64__
	__asm__ __volatile__ ("syscall" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2),
						  "d"(a3) : "rcx", "r11", "memory");
#endif
	return ret;
}

#define SYS_lseek				8

static off_t lseek(int fd, off_t offset, int whence)
{
#ifdef SYS__llseek
	off_t result;
	return syscall(SYS__llseek, fd, offset>>32, offset, &result, whence) ? -1 : result;
#else
	return __syscall3(SYS_lseek, fd, offset, whence);
#endif
}


int lkl_disk_add(union lkl_disk disk)
{
	struct virtio_blk_dev *dev;
	unsigned long long capacity;
	int ret;
	static int count;

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
	dev->disk = disk;

	ret = dev->ops->get_capacity(disk, &capacity);
	if (ret) {
		ret = ENOMEM;
		goto out_free;
	}
	dev->config.capacity = capacity;

	ret = virtio_dev_setup(&dev->dev, 1, 32);
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
int fd_get_capacity(union lkl_disk bs, unsigned long long *res)
{
	off_t off;

	off = lseek(bs.fd, 0, SEEK_END);
	if (off < 0)
		return -1;

	*res = off;
	return 0;
}

static int blk_request(union lkl_disk disk, struct lkl_blk_req *req)
{
	int err = 0;
	struct iovec *iovec = (struct iovec *)req->buf;

	/* TODO: handle short reads/writes */
	switch (req->type) {
	case LKL_DEV_BLK_TYPE_READ:
		err = preadv(disk.fd, iovec, req->count, req->sector * 512);
		break;
	case LKL_DEV_BLK_TYPE_WRITE:
		err = pwritev(disk.fd, iovec, req->count, req->sector * 512);
		break;
	case LKL_DEV_BLK_TYPE_FLUSH:
	case LKL_DEV_BLK_TYPE_FLUSH_OUT:
#ifdef __linux__
		err = fdatasync(disk.fd);
#else
		err = fsync(disk.fd);
#endif
		break;
	default:
		return LKL_DEV_BLK_STATUS_UNSUP;
	}

	if (err < 0)
		return LKL_DEV_BLK_STATUS_IOERR;

	return LKL_DEV_BLK_STATUS_OK;
}

struct lkl_dev_blk_ops lkl_dev_blk_ops = {
	.get_capacity = fd_get_capacity,
	.request = blk_request,
};
