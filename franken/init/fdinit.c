#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "init.h"

#ifdef MUSL_LIBC
#include <lkl.h>
#endif

extern char **environ;

struct lkl_netdev *lkl_netdev_rumpfd_create(const char *ifname, int fd);

/* FIXME: from sys/mount.h */
#define MS_RDONLY	 1	/* Mount read-only */

static int disk_id;

enum rump_etfs_type {
	RUMP_ETFS_REG,
	RUMP_ETFS_BLK,
	RUMP_ETFS_CHR,
	RUMP_ETFS_DIR,
	RUMP_ETFS_DIR_SUBDIRS
};

int rump_pub_etfs_register(const char *, const char *, enum rump_etfs_type);
int rump_pub_etfs_register_withsize(const char *, const char *, enum rump_etfs_type, uint64_t, uint64_t);
int rump_pub_etfs_remove(const char *);

int rump_pub_netconfig_ifcreate(const char *) __attribute__ ((weak));
int rump_pub_netconfig_dhcp_ipv4_oneshot(const char *) __attribute__ ((weak));
int rump_pub_netconfig_auto_ipv6(const char *) __attribute__ ((weak));
int rump_pub_netconfig_ifup(const char *) __attribute__ ((weak));
int rump_pub_netconfig_ipv4_ifaddr(const char *, const char *, const char *) __attribute__ ((weak));
int rump_pub_netconfig_ipv4_ifaddr_cidr(const char *, const char *, int) __attribute__ ((weak));
int rump_pub_netconfig_ipv4_gw(const char *) __attribute__ ((weak));

int rump_pub_netconfig_ifcreate(const char *interface) {return 0;}
int rump_pub_netconfig_dhcp_ipv4_oneshot(const char *interface) {return 0;}
int rump_pub_netconfig_auto_ipv6(const char *interface) {return 0;}
int rump_pub_netconfig_ifup(const char *interface) {return 0;}
int rump_pub_netconfig_ipv4_ifaddr(const char *interface, const char *addr, const char *mask) {return 0;}
int rump_pub_netconfig_ipv4_ifaddr_cidr(const char *interface, const char *addr, int mask) {return 0;};
int rump_pub_netconfig_ipv4_gw(const char *interface) {return 0;}

struct __fdtable __franken_fd[MAXFD];

void
mkkey(char *k, char *n, const char *pre, int dev, int fd)
{
	int i, d;
	int len = strlen(pre);

	if (fd > 99 || dev > 99) abort();
	for (i = 0; i < len; i++)
		*k++ = pre[i];
	if (dev > 9) {
		d = (dev / 10) + '0';
		*k++ = d;
		dev /= 10;
	}
	d = dev + '0';
	*k++ = d;
	*k++ = 0;

	if (fd > 9) {
		d = (fd / 10) + '0';
		*n++ = d;
		fd /= 10;
	}
	d = fd + '0';
	*n++ = d;
	*n++ = 0;
}

#ifdef MUSL_LIBC
#include <lkl_config.h>
#include <fcntl.h>
static struct lkl_config *json_cfg;

static void
franken_lkl_load_json_config(int fd)
{
	int ret;
	char buf[4096];

	json_cfg = (struct lkl_config *)malloc(sizeof(struct lkl_config));
	if (!json_cfg) {
		printf("malloc error\n");
		return;
	}
	memset(json_cfg, 0, sizeof(struct lkl_config));

	ret = read(fd, buf, sizeof(buf));
	if (ret == sizeof(buf))
		printf("too long json conf \n\n%s\n", buf);

	ret = lkl_load_config_json(json_cfg, buf);
	if (ret < 0)
		printf("load_config_json error \n\n%s\n", buf);

}

struct lkl_config *
franken_lkl_get_json_config(void)
{
	return json_cfg;
}
#endif

void __franken_fdinit()
{
	int n, fd;
	size_t x;
	char *env, *var;
	struct stat st;

	/* retrieve fd from environ embedded by rexec. */

	for (n = 0; n <= 2; n++) {
		memset(&st, 0, sizeof(struct stat));
		if (fstat(n, &st) == -1) {
			__franken_fd[n].valid = 0;
			continue;
		}

		/* make STDIN, STDOUT, STDERR valid */
		__franken_fd[n].valid = 1;
		__franken_fd[n].flags = fcntl(n, F_GETFL, 0);
		memcpy(&__franken_fd[n].st, &st, sizeof(struct stat));

		__franken_fd[n].seek = 0;
	}

        for (n = 0, env = *environ; env; env = *(environ + n++)) {

		/* XXX* useing strchr() causes undefined reference... */
		for (var = NULL, x = 0; x < strlen(env); x++) {
			if (env[x] == '=') {
				var = &env[x];
				break;
			}
		}
		if (var)
			var += 1;
		else
			continue;

		fd = atoi(var);
		if (strncmp(env, "__RUMP_FDINFO_", 14) == 0||
		    strncmp(env, "9PFS_FD", 7) == 0) {
			__franken_fd[fd].valid = 1;
			__franken_fd[fd].flags = fcntl(fd, F_GETFL, 0);

			memset(&st, 0, sizeof(struct stat));
			if (fstat(fd, &st) == -1) {
				__franken_fd[n].valid = 0;
			}
			memcpy(&__franken_fd[fd].st, &st, sizeof(struct stat));
		}

		if (strncmp(env, "__RUMP_FDINFO_CONFIGJSON", 24) == 0) {
			/* fd for config json */
			franken_lkl_load_json_config(fd);
		}

		if (strncmp(env, "__RUMP_FDINFO_ROOT", 18) == 0 ||
		    strncmp(env, "__RUMP_FDINFO_DISK_", 18) == 0) {
			__franken_fd[fd].seek = 1;
#ifdef MUSL_LIBC
			/* notify virtio-mmio dev id */
                        struct lkl_disk disk;
                        disk.ops = NULL;
                        disk.dev = NULL;
                        disk.fd = fd;
                        disk_id = lkl_disk_add(&disk);
#endif
		}

		if (strncmp(env, "__RUMP_FDINFO_NET_", 18) == 0) {
			/* fd for network i/o.
			 * create lkl_netdev for this rumpfd.
			 */
			__franken_fd[fd].seek = 0;
			lkl_netdev_rumpfd_create(NULL, fd);
		}

#ifdef MUSL_LIBC
		if (strncmp(env, "9PFS_FD", 7) == 0) {
			__franken_fd[fd].seek = 0;

			int lkl_9pfs_add(struct lkl_9pfs *fs);

			char *tmpfd = getenv("9PFS_FD");
			struct lkl_9pfs fs;
			fs.ops = NULL;
			fs.dev = NULL;
			fs.fd = atoi(tmpfd);
			lkl_9pfs_add(&fs);
#endif
		}
	}
}



/* XXX would be much nicer to build these functions against NetBSD libc headers, but at present
   they are not built, or installed, yet. Need to reorder the build process */

#define IOCPARM_MASK    0x1fff
#define IOCPARM_SHIFT   16
#define IOCGROUP_SHIFT  8
#define IOCPARM_LEN(x)  (((x) >> IOCPARM_SHIFT) & IOCPARM_MASK)
#define IOCBASECMD(x)   ((x) & ~(IOCPARM_MASK << IOCPARM_SHIFT))
#define IOCGROUP(x)     (((x) >> IOCGROUP_SHIFT) & 0xff)

#define IOC_VOID        (unsigned long)0x20000000
#define IOC_OUT         (unsigned long)0x40000000
#define IOC_IN          (unsigned long)0x80000000
#define IOC_INOUT       (IOC_IN|IOC_OUT)
#define IOC_DIRMASK     (unsigned long)0xe0000000

#define _IOC(inout, group, num, len) \
    ((inout) | (((len) & IOCPARM_MASK) << IOCPARM_SHIFT) | \
    ((group) << IOCGROUP_SHIFT) | (num))
#define _IO(g,n)        _IOC(IOC_VOID,  (g), (n), 0)
#define _IOR(g,n,t)     _IOC(IOC_OUT,   (g), (n), sizeof(t))
#define _IOW(g,n,t)     _IOC(IOC_IN,    (g), (n), sizeof(t))
#define _IOWR(g,n,t)    _IOC(IOC_INOUT, (g), (n), sizeof(t))

struct ufs_args {
	char *fspec;
};

struct tmpfs_args {
	int ta_version;
	ino_t ta_nodes_max;
	off_t ta_size_max;
	uid_t ta_root_uid;
	gid_t ta_root_gid;
	mode_t ta_root_mode;
};

#define MNT_RDONLY	0x00000001
#define MNT_LOG		0x02000000
#define MNT_FORCE	0x00080000

#define IFNAMSIZ 16

struct ifcapreq {
	char		ifcr_name[IFNAMSIZ];
	uint64_t	ifcr_capabilities;
	uint64_t	ifcr_capenable;
};

#define SIOCGIFCAP	_IOWR('i', 118, struct ifcapreq)
#define SIOCSIFCAP	_IOW('i', 117, struct ifcapreq)

int rump___sysimpl_open(const char *, int, ...);
int rump___sysimpl_close(int);
int rump___sysimpl_dup2(int, int);
int rump___sysimpl_mount50(const char *, const char *, int, void *, size_t);
int rump___sysimpl_unmount(const char *, int);
int rump___sysimpl_socket30(int, int, int);
int rump___sysimpl_mkdir(const char *, mode_t);
int rump___sysimpl_ioctl(int, unsigned long, void *);

#define AF_UNSPEC 0
#define AF_INET 2
#define AF_INET6 24
#define SOCK_STREAM 1

static void
mount_tmpfs(void)
{
	struct tmpfs_args ta = {
		.ta_version = 1,
		.ta_nodes_max = 0,
		.ta_size_max = 0,
		.ta_root_uid = 0,
		.ta_root_gid = 0,
		.ta_root_mode = 0777,
	};

	/* try to mount tmpfs if possible, useful for ro root fs */
	rump___sysimpl_mkdir("/tmp", 0777);
	/* might fail if no /tmp directory, or tmpfs not included in system */
	rump___sysimpl_mount50("tmpfs", "/tmp", 0, &ta, sizeof(struct tmpfs_args));
}

static void
unmount_atexit(void)
{
	int ret __attribute__((__unused__));

	ret = rump___sysimpl_unmount("/", MNT_FORCE);
}

static int
register_reg(int dev, int fd, int flags)
{
	char key[16], num[16];
#ifdef MUSL_LIBC
	if ((__franken_fd[fd].st.st_mode & S_IFMT) == S_IFREG)
		return fd;
#endif

#ifndef MUSL_LIBC
	mkkey(key, num, "/dev/vfile", dev, fd);
#else
	memcpy(key, "/dev/console", 13);
#endif
	rump_pub_etfs_register(key, num, RUMP_ETFS_REG);
	return rump___sysimpl_open(key, flags);
}

static void
register_net(int fd)
{
#ifndef MUSL_LIBC
	char *addr, *mask, *gw;
	char key[16], num[16];
	int ret;
	int sock;
	int af = AF_UNSPEC;
	struct ifcapreq cap;

	mkkey(key, num, "virt", fd, fd);
	ret = rump_pub_netconfig_ifcreate(key);
	if (ret == 0) {
		sock = rump___sysimpl_socket30(AF_INET6, SOCK_STREAM, 0);
		if (sock != -1) {
			rump_pub_netconfig_auto_ipv6(key);
			rump___sysimpl_close(sock);
			af = AF_INET6;
		}
		sock = rump___sysimpl_socket30(AF_INET, SOCK_STREAM, 0);
		if (sock != -1) {
			/* XXX move to autodetect later, but gateway complex */
			addr = getenv("FIXED_ADDRESS");
			mask = getenv("FIXED_MASK");
			gw = getenv("FIXED_GATEWAY");
			if (addr == NULL || mask == NULL || gw == NULL) {
				rump_pub_netconfig_dhcp_ipv4_oneshot(key);
			} else {
				rump_pub_netconfig_ifup(key);
				rump_pub_netconfig_ipv4_ifaddr_cidr(key, addr, atoi(mask));
				rump_pub_netconfig_ipv4_gw(gw);
			}
			rump___sysimpl_close(sock);
			af = AF_INET;
		}
		/* enable offload features */
		sock = rump___sysimpl_socket30(af, SOCK_STREAM, 0);
		if (sock != -1) {
			strncpy(&cap.ifcr_name, key, IFNAMSIZ);
			ret = rump___sysimpl_ioctl(sock, SIOCGIFCAP, &cap);
			if (ret != -1) {
				cap.ifcr_capenable = cap.ifcr_capabilities;
				rump___sysimpl_ioctl(sock, SIOCSIFCAP, &cap);
			}
			rump___sysimpl_close(sock);
		}
	}
#endif
}

static int
register_block(int dev, int fd, int flags, off_t size, int root)
{
#ifdef MUSL_LIBC
	/* FIXME: hehe always fixme tagged.. */
	int ret;
	char mnt_point[32];

	ret = lkl_mount_dev(disk_id, 0, "ext4", 0, NULL, mnt_point,
			    sizeof(mnt_point));
	if (ret < 0) {
		ret = lkl_mount_dev(disk_id, 0, "iso9660", MS_RDONLY, NULL,
				    mnt_point, sizeof(mnt_point));
	}
	if (ret < 0)
		printf("can't mount disk (%d) at %s. err=%d\n",
			disk_id, mnt_point, ret);
	/* chroot to the mounted volume (and create minimal fs) */
	ret = lkl_sys_chroot(mnt_point);
	if (ret) {
		printf("can't chroot to %s: %s\n", mnt_point,
		       lkl_strerror(ret));
	}
	ret = lkl_sys_chdir("/");
	if (ret) {
		printf("can't chdir to %s: %s\n", mnt_point,
		       lkl_strerror(ret));
	}

	ret = lkl_sys_mkdir("/dev/", 0700);
	if (ret < 0 && ret != -EEXIST) {
		printf("can't mkdir /dev to: %s\n",
		       lkl_strerror(ret));
	}
	ret = lkl_sys_mknod("/dev/null", LKL_S_IFCHR | 0600, LKL_MKDEV(1, 3));
	if (ret < 0 && ret != -EEXIST) {
		printf("can't mknod /dev/null to: %s\n",
		       lkl_strerror(ret));
	}

	ret = lkl_sys_mknod("/dev/urandom", LKL_S_IFCHR | 0600, LKL_MKDEV(1, 9));
	if (ret < 0 && ret != -EEXIST) {
		printf("can't mknod /dev/urandom to: %s\n",
		       lkl_strerror(ret));
	}

	atexit(unmount_atexit);
	return ret;
#else
	char key[16], rkey[16], num[16];
	struct ufs_args ufs;
	int ret;

	mkkey(key, num, "/dev/block", dev, fd);
	mkkey(rkey, num, "/dev/rblock", dev, fd);
	rump_pub_etfs_register_withsize(key, num, RUMP_ETFS_BLK, 0, size);
	rump_pub_etfs_register_withsize(rkey, num, RUMP_ETFS_CHR, 0, size);
	if (root == 0)
		return 0;
	ufs.fspec = key;
	if (flags == O_RDWR)
		flags = MNT_LOG;
	else
		flags = MNT_RDONLY;
	ret = rump___sysimpl_mount50("ffs", "/", flags, &ufs, sizeof(struct ufs_args));
	if (ret == -1) {
		if (flags == MNT_LOG)
			flags = 0;
		ret = rump___sysimpl_mount50("ext2fs", "/", flags, &ufs, sizeof(struct ufs_args));
	}
	if (ret == 0)
		atexit(unmount_atexit);
	return ret;
#endif
}


void
__franken_fdinit_create()
{
	int fd, ret;
	int n_reg = 0, n_block = 0;
	char *env;

	if (__franken_fd[0].valid) {
		fd = register_reg(n_reg++, 0, O_RDONLY);
		if (fd != -1) {
			rump___sysimpl_dup2(fd, 0);
			rump___sysimpl_close(fd);
		}
	}
	if (__franken_fd[1].valid) {
		fd = register_reg(n_reg++, 1, O_WRONLY);
		if (fd != -1) {
			rump___sysimpl_dup2(fd, 1);
			rump___sysimpl_close(fd);
		}
	}

	if (__franken_fd[2].valid) {
		fd = register_reg(n_reg++, 2, O_WRONLY);
		if (fd != -1) {
			rump___sysimpl_dup2(fd, 2);
			rump___sysimpl_close(fd);
		}
	}

	/* XXX would be nicer to be able to detect a file system,
	   but this also allows us not to mount a block device.
	   Pros and cons, may change if this is not convenient */


	/* find rootfs fd and make it mounted */
	env = getenv("__RUMP_FDINFO_ROOT");
	if (env) {
		fd = atoi(env);
		if (__franken_fd[fd].valid) {
			ret = register_block(n_block++, fd,
					     __franken_fd[fd].flags
					     & O_ACCMODE,
					     __franken_fd[fd].st.st_size, 1);
			if (ret == 0)
				__franken_fd[fd].mounted = 1;
		}
	}


	for (fd = 5; fd < MAXFD; fd++) {
		if (__franken_fd[fd].valid == 0)
			break;
		switch (__franken_fd[fd].st.st_mode & S_IFMT) {
		case S_IFREG:
			fd = register_reg(n_reg++, fd,
					  __franken_fd[fd].flags & O_ACCMODE);
			break;
		case S_IFBLK:
			register_block(n_block++, fd,
				       __franken_fd[fd].flags & O_ACCMODE,
				       __franken_fd[fd].st.st_size, 0);
			break;
		case S_IFSOCK:
			register_net(fd);
			break;
		}
	}

#ifdef MUSL_LIBC
	/* mount 9pfs */
	if (getenv("9PFS_MNT")) {
		char *mnt_point = getenv("9PFS_MNT");
		int ret;

		if (strcmp(mnt_point, "/") == 0) {
			ret = lkl_sys_mkdir("/mnt", 0700);
			ret = lkl_sys_mount("", "/mnt", "9p", 0,
					    "trans=virtio,cache=mmap");
			if (ret < 0)
				printf("can't mount 9p fs err=%d\n", ret);

			ret = lkl_sys_chroot("/mnt");
			if (ret) {
				printf("can't chdir to %s: %s\n", mnt_point,
				       lkl_strerror(ret));
			}
			ret = lkl_sys_chdir("/");
			if (ret) {
				printf("can't chdir to %s: %s\n", mnt_point,
				       lkl_strerror(ret));
			}
		}
		else {
			ret = lkl_sys_mkdir(mnt_point, 0700);
			ret = lkl_sys_mount("", mnt_point, "9p", 0,
					    "trans=virtio,cache=mmap");
			if (ret < 0)
				printf("can't mount 9p fs err=%d\n", ret);
		}
	}
#endif

	/* now some generic stuff */
	mount_tmpfs();
	/* mount procfs */
	rump___sysimpl_mkdir("/proc", 0777);
	rump___sysimpl_mount50("proc", "/proc", 0, NULL, 0);
	/* mount sysfs */
	rump___sysimpl_mkdir("/sys", 0777);
	rump___sysimpl_mount50("sysfs", "/sys", 0, NULL, 0);
}

void franken_recv_thread(int fd, void *thrid)
{
	__franken_fd[fd].wake = thrid;
}
