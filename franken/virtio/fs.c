#include <stdio.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <fcntl.h>

/* XXX: temporary solution */
#define __user
#define S_IRWXO 00007
typedef unsigned short		umode_t;

long lkl_syscall(long no, long *params);
#define __LKL__MAP0(m,...)
#define __LKL__MAP1(m,t,a) m(t,a)
#define __LKL__MAP2(m,t,a,...) m(t,a), __LKL__MAP1(m,__VA_ARGS__)
#define __LKL__MAP3(m,t,a,...) m(t,a), __LKL__MAP2(m,__VA_ARGS__)
#define __LKL__MAP4(m,t,a,...) m(t,a), __LKL__MAP3(m,__VA_ARGS__)
#define __LKL__MAP5(m,t,a,...) m(t,a), __LKL__MAP4(m,__VA_ARGS__)
#define __LKL__MAP6(m,t,a,...) m(t,a), __LKL__MAP5(m,__VA_ARGS__)
#define __LKL__MAP(n,...) __LKL__MAP##n(__VA_ARGS__)

#define __LKL__SC_LONG(t, a) (long)a
#define __LKL__SC_DECL(t, a) t a

#define LKL_SYSCALL0(name)					       \
	static __inline__ long lkl_sys##name(void)			       \
	{							       \
		long params[6];					       \
		return lkl_syscall(__lkl__NR##name, params);	       \
	}

#define LKL_SYSCALLx(x, name, ...)				       \
       	static __inline__						       \
	long lkl_sys##name(__LKL__MAP(x, __LKL__SC_DECL, __VA_ARGS__))	       \
	{							       \
		long params[6] = { __LKL__MAP(x, __LKL__SC_LONG, __VA_ARGS__) }; \
		return lkl_syscall(__lkl__NR##name, params);	       \
	}

#define LKL_SYSCALL_DEFINE0(name, ...) LKL_SYSCALL0(name)
#define LKL_SYSCALL_DEFINE1(name, ...) LKL_SYSCALLx(1, name, __VA_ARGS__)
#define LKL_SYSCALL_DEFINE2(name, ...) LKL_SYSCALLx(2, name, __VA_ARGS__)
#define LKL_SYSCALL_DEFINE3(name, ...) LKL_SYSCALLx(3, name, __VA_ARGS__)
#define LKL_SYSCALL_DEFINE4(name, ...) LKL_SYSCALLx(4, name, __VA_ARGS__)
#define LKL_SYSCALL_DEFINE5(name, ...) LKL_SYSCALLx(5, name, __VA_ARGS__)
#define LKL_SYSCALL_DEFINE6(name, ...) LKL_SYSCALLx(6, name, __VA_ARGS__)

#define __lkl__NR_mkdir 1030
#ifdef __lkl__NR_mkdir
LKL_SYSCALL_DEFINE2(_mkdir,const char *,pathname,umode_t,mode)
#endif
#define __lkl__NR_rmdir 1031
#ifdef __lkl__NR_rmdir
LKL_SYSCALL_DEFINE1(_rmdir,const char *,pathname)
#endif
#define __lkl__NR_open 1024
#ifdef __lkl__NR_open
LKL_SYSCALL_DEFINE3(_open,const char *,filename,int,flags,umode_t,mode)
#endif
#define __lkl__NR_close 57
#ifdef __lkl__NR_close
LKL_SYSCALL_DEFINE1(_close,unsigned int,fd)
#endif
#define __lkl__NR_unlink 1026
#ifdef __lkl__NR_unlink
LKL_SYSCALL_DEFINE1(_unlink,const char *,pathname)
#endif
#define __lkl__NR_access 1033
#ifdef __lkl__NR_access
LKL_SYSCALL_DEFINE2(_access,const char *,filename,int,mode)
#endif
#define __lkl__NR_mount 40
#ifdef __lkl__NR_mount
LKL_SYSCALL_DEFINE5(_mount,char *,dev_name,char *,dir_name,char *,type,unsigned long,flags,void *,data)
#endif
#define __lkl__NR_umount 1076
#ifdef __lkl__NR_umount
LKL_SYSCALL_DEFINE2(_umount,char *,name,int,flags)
#endif
#define __lkl__NR_mknod 1027
#ifdef __lkl__NR_mknod
LKL_SYSCALL_DEFINE3(_mknod,const char *,filename,umode_t,mode,unsigned,dev)
#endif
#define __lkl__NR_read 63
#ifdef __lkl__NR_read
LKL_SYSCALL_DEFINE3(_read,unsigned int,fd,char *,buf,size_t,count)
#endif

#define LKL_MKDEV(ma,mi)	((ma)<<8 | (mi))

long lkl_mount_sysfs(void)
{
	long ret;
	static int sysfs_mounted;

	if (sysfs_mounted)
		return 0;

	ret = lkl_sys_mkdir("/sys", 0700);
	if (ret)
		return ret;

	ret = lkl_sys_mount("none", "sys", "sysfs", 0, NULL);

	if (ret == 0)
		sysfs_mounted = 1;

	return ret;
}

static long get_virtio_blkdev(int disk_id)
{
	char sysfs_path[] = "/sys/block/vda/dev";
	char buf[16] = { 0, };
	long fd, ret;
	int major, minor;

	ret = lkl_mount_sysfs();
	if (ret)
		return ret;

	sysfs_path[strlen("/sys/block/vd")] += disk_id;

	fd = lkl_sys_open(sysfs_path, O_RDONLY, 0);
	if (fd < 0)
		return fd;

	ret = lkl_sys_read(fd, buf, sizeof(buf));
	if (ret < 0)
		goto out_close;

	if (ret == sizeof(buf)) {
		ret = -ENOBUFS;
		goto out_close;
	}

	ret = sscanf(buf, "%d:%d", &major, &minor);
	if (ret != 2) {
		ret = -EINVAL;
		goto out_close;
	}

	ret = LKL_MKDEV(major, minor);

out_close:
	lkl_sys_close(fd);

	return ret;
}

long lkl_mount_dev(unsigned int disk_id, const char *fs_type, int flags,
		   void *data, char *mnt_str, unsigned int mnt_str_len)
{
	char dev_str[] = { "/dev/xxxxxxxx" };
	unsigned int dev;
	int err;

	if (mnt_str_len < sizeof(dev_str))
		return -ENOMEM;

	dev = get_virtio_blkdev(disk_id);

	snprintf(dev_str, sizeof(dev_str), "/dev/%08x", dev);
//	snprintf(mnt_str, mnt_str_len, "/mnt/%08x", dev);

	err = lkl_sys_access("/dev", S_IRWXO);
	if (err < 0) {
		if (err == -ENOENT)
			err = lkl_sys_mkdir("/dev", 0700);
		if (err < 0)
			return err;
	}

	err = lkl_sys_mknod(dev_str, S_IFBLK | 0600, dev);
	if (err < 0)
		return err;

	err = lkl_sys_access(mnt_str, S_IRWXO);
	if (err < 0) {
		if (err == -ENOENT)
			err = lkl_sys_mkdir(mnt_str, 0700);
		if (err < 0)
			return err;
	}
#if 0 //ndef LIBRUMPUSER
	err = lkl_sys_mkdir(mnt_str, 0700);
	if (err < 0) {
		lkl_sys_unlink(dev_str);
		return err;
	}
#endif
	err = lkl_sys_mount(dev_str, mnt_str, (char*)fs_type, flags, data);
	if (err < 0) {
		lkl_sys_unlink(dev_str);
		lkl_sys_rmdir(mnt_str);
		return err;
	}

	/* FIXME: move somewhere else */
	err = lkl_sys_mkdir("/tmp", 0700);
	err = lkl_sys_mkdir("/etc", 0700);
	err = lkl_sys_mkdir("/usr", 0700);
	err = lkl_sys_mkdir("/usr/local", 0700);
	err = lkl_sys_mkdir("/usr/local/nginx", 0700);
	err = lkl_sys_mkdir("/usr/local/nginx/logs", 0700);

	return 0;
}

long lkl_umount_dev(char *mnt_str, int flags)
{
	int err;

	err = lkl_sys_umount(mnt_str, flags);

	return err;
}

#if 0
long lkl_umount_dev(unsigned int disk_id, int flags, long timeout_ms)
{
	char dev_str[] = { "/dev/xxxxxxxx" };
	char mnt_str[] = { "/mnt/xxxxxxxx" };
	long incr = 10000000; /* 10 ms */
	struct lkl_timespec ts = {
		.tv_sec = 0,
		.tv_nsec = incr,
	};
	unsigned int dev;
	int err;

	dev = get_virtio_blkdev(disk_id);

	snprintf(dev_str, sizeof(dev_str), "/dev/%08x", dev);
	snprintf(mnt_str, sizeof(mnt_str), "/mnt/%08x", dev);

	do {
		err = lkl_sys_umount(mnt_str, flags);
		if (err == -LKL_EBUSY) {
			lkl_sys_nanosleep(&ts, NULL);
			timeout_ms -= incr / 1000000;
		}
	} while (err == -LKL_EBUSY && timeout_ms > 0);

	if (err)
		return err;

	err = lkl_sys_unlink(dev_str);
	if (err)
		return err;

	return lkl_sys_rmdir(mnt_str);
}

struct lkl_dir {
	int fd;
	char buf[1024];
	char *pos;
	int len;
};

struct lkl_dir *lkl_opendir(const char *path, int *err)
{
	struct lkl_dir *dir = malloc(sizeof(struct lkl_dir));

	if (!dir) {
		*err = -LKL_ENOMEM;
		return NULL;
	}

	dir->fd = lkl_sys_open(path, LKL_O_RDONLY | LKL_O_DIRECTORY, 0);
	if (dir->fd < 0) {
		*err = dir->fd;
		free(dir);
		return NULL;
	}

	dir->len = 0;
	dir->pos = NULL;

	return dir;
}

int lkl_closedir(struct lkl_dir *dir)
{
	int ret;

	ret = lkl_sys_close(dir->fd);
	free(dir);

	return ret;
}

struct lkl_linux_dirent64 *lkl_readdir(struct lkl_dir *dir)
{
	struct lkl_linux_dirent64 *de;

	if (dir->len < 0)
		return NULL;

	if (!dir->pos || dir->pos - dir->buf >= dir->len)
		goto read_buf;

return_de:
	de = (struct lkl_linux_dirent64 *)dir->pos;
	dir->pos += de->d_reclen;

	return de;

read_buf:
	dir->pos = NULL;
	de = (struct lkl_linux_dirent64 *)dir->buf;
	dir->len = lkl_sys_getdents64(dir->fd, de, sizeof(dir->buf));
	if (dir->len <= 0)
		return NULL;

	dir->pos = dir->buf;
	goto return_de;
}

int lkl_errdir(struct lkl_dir *dir)
{
	if (dir->len >= 0)
		return 0;

	return dir->len;
}

int lkl_dirfd(struct lkl_dir *dir)
{
	return dir->fd;
}
#endif
