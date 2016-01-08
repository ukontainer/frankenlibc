#include <string.h>
#include <thread.h>
#include <stddef.h>
#include <errno.h>
#include <sys/errno.h>
#include <../linux/linux.h>	/* in platform/ */
#include "virtio.h"
#include "endian.h"
#include "thread.h"
#include "init.h"

#include "../../lkl-linux/include/linux/virtio_mmio.h"
#include "../../lkl-linux/include/uapi/linux/virtio_net.h"

#define BIT(x) (1<<x)

/* XXX: copied from lkl.h */
union lkl_netdev {
	int fd;
};

struct virtio_net_poll {
	struct virtio_net_dev *dev;
	void *sem;
	int event;
	struct thread *rcvthr;
};

struct lkl_dev_net_ops {
	int (*tx)(union lkl_netdev nd, void *data, int len);
	int (*rx)(union lkl_netdev nd, void *data, int *len);
#define LKL_DEV_NET_POLL_RX		1
#define LKL_DEV_NET_POLL_TX		2
	int (*poll)(union lkl_netdev nd, int events);
};

struct virtio_net_dev {
	struct virtio_dev dev;
	const char offset[VIRTIO_MMIO_CONFIG - sizeof(struct virtio_dev)];
	struct virtio_net_config config;
	struct lkl_dev_net_ops *ops;
	union lkl_netdev nd;
	struct virtio_net_poll rx_poll, tx_poll;
};

#define VIRTIO_NET_HDR_F_NEEDS_CSUM    1
#define VIRTIO_NET_HDR_GSO_NONE        0
#define VIRTIO_NET_HDR_GSO_TCPV4       1
#define VIRTIO_NET_HDR_GSO_UDP         3
#define VIRTIO_NET_HDR_GSO_TCPV6       4
#define VIRTIO_NET_HDR_GSO_ECN      0x80

struct virtio_net_header {
        uint8_t flags;
        uint8_t gso_type;
        uint16_t hdr_len;
        uint16_t gso_size;
        uint16_t csum_start;
        uint16_t csum_offset;
        uint16_t num_buffers;
	char data[];
} __attribute((packed));

/* XXX: should be defined in a common place somewhere else */
struct rumpuser_sem {
	struct rumpuser_mtx *lock;
	int count;
	struct rumpuser_cv *cond;
};

static void *franken_sem_alloc(int count)
{
	struct rumpuser_sem *sem;

	sem = malloc(sizeof(*sem));
	if (!sem)
		return NULL;

	mutex_init(&sem->lock, RUMPUSER_MTX_SPIN);
	sem->count = count;
	cv_init(&sem->cond);

	return sem;
}

static void franken_sem_free(void *_sem)
{
	struct rumpuser_sem *sem = (struct rumpuser_sem *)_sem;

	cv_destroy(sem->cond);
	mutex_destroy(sem->lock);
	free(sem);
}

static void franken_sem_up(void *_sem)
{
	struct rumpuser_sem *sem = (struct rumpuser_sem *)_sem;

	mutex_enter(sem->lock);
	sem->count++;
	if (sem->count > 0)
		cv_signal(sem->cond);
	mutex_exit(sem->lock);
}

static void franken_sem_down(void *_sem)
{
	struct rumpuser_sem *sem = (struct rumpuser_sem *)_sem;

	mutex_enter(sem->lock);
	while (sem->count <= 0)
		cv_wait(sem->cond, sem->lock);
	sem->count--;
	mutex_exit(sem->lock);
}


static int net_check_features(struct virtio_dev *dev)
{
	if (dev->driver_features == dev->device_features)
		return 0;

	return -EINVAL;
}

#if 0
static void net_notify(struct virtio_dev *dev, int qidx)
{
	if (qidx == 0)
		virtio_process_queue(dev, qidx);
}
#endif

static int net_enqueue(struct virtio_dev *dev, struct virtio_req *req)
{
	struct virtio_net_header *h;
	struct virtio_net_dev *net_dev;
	int ret, len;
	void *buf;

	h = req->buf[0].addr;
	net_dev = container_of(dev, struct virtio_net_dev, dev);
	len = req->buf[0].len - sizeof(*h);
	buf = h->data;

	if (!len && req->buf_count > 1) {
		buf = req->buf[1].addr;
		len = req->buf[1].len;
	}

	if (req->q != dev->queue) {
		ret = net_dev->ops->tx(net_dev->nd, buf, len);
		if (ret < 0) {
			franken_sem_up(net_dev->tx_poll.sem);
			return -1;
		}
	} else {
		h->num_buffers = 1;

		/* XXX: need to use this semantics for franken_poll(2) */
		__franken_fd[net_dev->nd.fd].wake = net_dev->rx_poll.rcvthr;
		ret = net_dev->ops->rx(net_dev->nd, buf, &len);
		if (ret < 0) {
			franken_sem_up(net_dev->rx_poll.sem);
			return -1;
		}
	}

	virtio_req_complete(req, len + sizeof(*h));
	return 0;
}

static struct virtio_dev_ops net_ops = {
	.check_features = net_check_features,
	.enqueue = net_enqueue,
};

void poll_thread(void *arg)
{
	struct virtio_net_poll *np = (struct virtio_net_poll *)arg;
	int ret;

	while ((ret = np->dev->ops->poll(np->dev->nd, np->event)) >= 0) {
		if (ret & LKL_DEV_NET_POLL_RX)
			virtio_process_queue(&np->dev->dev, 0);
		if (ret & LKL_DEV_NET_POLL_TX)
			virtio_process_queue(&np->dev->dev, 1);
		franken_sem_down(np->sem);
	}
}

static int net_tx(union lkl_netdev nd, void *data, int len)
{
	int ret;

	ret = write(nd.fd, data, len);
	if (ret <= 0 && errno == -EAGAIN)
		return -1;
	return 0;
}

static int net_rx(union lkl_netdev nd, void *data, int *len)
{
	int ret;

	ret = read(nd.fd, data, *len);
	if (ret <= 0)
		return -1;
	*len = ret;
	return 0;
}

/* XXX: move to platform/ or franken */
typedef unsigned long nfds_t;

static __inline long __syscall3(long n, long a1, long a2, long a3)
{
	unsigned long ret;
#ifdef __x86_64__
	__asm__ __volatile__ ("syscall" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2),
						  "d"(a3) : "rcx", "r11", "memory");
#endif
	return ret;
}

#define SYS_poll				7
static int franken_poll(struct pollfd *fds, nfds_t n, int timeout)
{
	return __syscall3(SYS_poll, (long)fds, n, timeout);
}

static int net_poll(union lkl_netdev nd, int events)
{
	struct pollfd pfd = {
		.fd = nd.fd,
	};
	int ret = 0;

	if (events & LKL_DEV_NET_POLL_RX)
		pfd.events |= POLLIN;
	if (events & LKL_DEV_NET_POLL_TX)
		pfd.events |= POLLOUT;

	while (1) {
		/* XXX: this should be poll(pfd, 1, -1) but fiber thread
		 * needs to be done like this...
		 */
		int err = franken_poll(&pfd, 1, 0);
		if (err < 0 && errno == EINTR)
			continue;
		if (err > 0)
			break;
		/* will be woken by poll */
		clock_sleep(CLOCK_REALTIME, 10, 0);
	}

	if (pfd.revents & (POLLHUP | POLLNVAL))
		return -1;

	if (pfd.revents & POLLIN)
		ret |= LKL_DEV_NET_POLL_RX;
	if (pfd.revents & POLLOUT)
		ret |= LKL_DEV_NET_POLL_TX;

	return ret;
}

struct lkl_dev_net_ops lkl_dev_net_ops = {
	.tx = net_tx,
	.rx = net_rx,
	.poll = net_poll,
};

int lkl_netdev_add(union lkl_netdev nd, void *mac)
{
	struct virtio_net_dev *dev;
	static int count;
	int ret = -ENOMEM;

	dev = malloc(sizeof(*dev));
	if (!dev)
		return -ENOMEM;

	dev->dev.device_id = 1;
	dev->dev.vendor_id = 0;
	dev->dev.device_features = 0;
	if (mac)
		dev->dev.device_features |= VIRTIO_NET_F_MAC;
	dev->dev.config_gen = 0;
	dev->dev.config_data = &dev->config;
	dev->dev.config_len = sizeof(dev->config);
	dev->dev.ops = &net_ops;
	dev->ops = &lkl_dev_net_ops;
	dev->nd = nd;

	if (mac)
		memcpy(dev->config.mac, mac, 6);

	dev->rx_poll.event = LKL_DEV_NET_POLL_RX;
	dev->rx_poll.sem = franken_sem_alloc(0);
	dev->rx_poll.dev = dev;

	dev->tx_poll.event = LKL_DEV_NET_POLL_TX;
	dev->tx_poll.sem = franken_sem_alloc(0);
	dev->tx_poll.dev = dev;

	if (!dev->rx_poll.sem || !dev->tx_poll.sem)
		goto out_free;

	ret = virtio_dev_setup(&dev->dev, 2, 32);
	if (ret)
		goto out_free;

	if ((dev->rx_poll.rcvthr = create_thread("virtio-net-rx", NULL,
						 (void (*)(void *))poll_thread,
						 &dev->rx_poll, NULL, 0, 1))
	    == NULL)
		goto out_cleanup_dev;

	if (create_thread("virtio-net-tx", NULL,
			  (void (*)(void *))poll_thread, &dev->tx_poll,
			  NULL, 0, 1) == NULL)
		goto out_cleanup_dev;

	/* RX/TX thread polls will exit when the host netdev handle is closed */

	return count++;

out_cleanup_dev:
	virtio_dev_cleanup(&dev->dev);

out_free:
	if (dev->rx_poll.sem)
		franken_sem_free(dev->rx_poll.sem);
	if (dev->tx_poll.sem)
		franken_sem_free(dev->tx_poll.sem);
	free(dev);

	return ret;
}
