/*
 * LKL network related utilities
 *
 * Picked from a code of lkl-linux-2.6
 * https://github.com/lkl/lkl-linux-2.6/blob/master/arch/lkl/envs/lib/net.c
 *
 * Author: Octavian Purdila
 *         Lucian Adrian Grijincu
 *         Hajime Tazaki <thehajime@gmail.com>
 *
 */


#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <linux/socket.h>
#include <sys/socket.h>
#include <linux/sockios.h>
#include <linux/route.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

static inline void
set_sockaddr(struct sockaddr_in *sin, unsigned int addr, unsigned short port)
{
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = addr;
	sin->sin_port = port;
}

static inline int ifindex_to_name(int sock, struct ifreq *ifr, int ifindex)
{
	ifr->ifr_ifindex = ifindex;
	return ioctl(sock, SIOCGIFNAME, (long)ifr);
}

int lkl_if_up(int ifindex)
{
	struct ifreq ifr;
	int err, sock = socket(PF_INET, SOCK_DGRAM, 0);

	if (sock < 0)
		return sock;

	err = ifindex_to_name(sock, &ifr, ifindex);
	if (err < 0)
		return err;

	err = ioctl(sock, SIOCGIFFLAGS, (long)&ifr);
	if (!err) {
		ifr.ifr_flags |= IFF_UP;
		err = ioctl(sock, SIOCSIFFLAGS, (long)&ifr);
	}

	close(sock);

	return err;
}

int lkl_if_down(int ifindex)
{
	struct ifreq ifr;
	int err, sock = socket(PF_INET, SOCK_DGRAM, 0);

	if (sock < 0)
		return sock;

	err = ifindex_to_name(sock, &ifr, ifindex);
	if (err < 0)
		return err;

	err = ioctl(sock, SIOCGIFFLAGS, (long)&ifr);
	if (!err) {
		ifr.ifr_flags &= ~IFF_UP;
		err = ioctl(sock, SIOCSIFFLAGS, (long)&ifr);
	}

	close(sock);

	return err;
}

int lkl_if_set_ipv4(int ifindex, unsigned int addr, unsigned int netmask_len)
{
	struct ifreq ifr;
	struct sockaddr_in *sin = (struct sockaddr_in *)&ifr.ifr_addr;
	int err, sock = socket(PF_INET, SOCK_DGRAM, 0);

	if (sock < 0)
		return sock;

	err = ifindex_to_name(sock, &ifr, ifindex);
	if (err < 0)
		return err;

	if (netmask_len >= 31)
		return -EINVAL;

	set_sockaddr(sin, addr, 0);
	err = ioctl(sock, SIOCSIFADDR, (long)&ifr);
	if (!err) {
		int netmask = (((1<<netmask_len)-1))<<(32-netmask_len);

		set_sockaddr(sin, htonl(netmask), 0);
		err = ioctl(sock, SIOCSIFNETMASK, (long)&ifr);
		if (!err) {
			set_sockaddr(sin, htonl(ntohl(addr)|~netmask), 0);
			err = ioctl(sock, SIOCSIFBRDADDR, (long)&ifr);
		}
	}

	close(sock);

	return err;
}


int lkl_set_gateway(unsigned int addr)
{
	struct rtentry re;
	int err, sock = socket(PF_INET, SOCK_DGRAM, 0);

	if (sock < 0)
		return sock;

	memset(&re, 0, sizeof(re));
	set_sockaddr((struct sockaddr_in *) &re.rt_dst, 0, 0);
	set_sockaddr((struct sockaddr_in *) &re.rt_genmask, 0, 0);
	set_sockaddr((struct sockaddr_in *) &re.rt_gateway, addr, 0);
	re.rt_flags = RTF_UP | RTF_GATEWAY;
	err = ioctl(sock, SIOCADDRT, (long)&re);
	close(sock);

	return err;
}

int lkl_netdev_get_ifindex(int id)
{
	struct ifreq ifr;
	int sock, ret;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0)
		return sock;

	snprintf(ifr.ifr_name, sizeof(ifr.ifr_name), "eth%d", id);
	ret = ioctl(sock, SIOCGIFINDEX, (long)&ifr);
	close(sock);

	return ret < 0 ? ret : ifr.ifr_ifindex;
}

