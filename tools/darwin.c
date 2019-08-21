#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/disk.h>
#include <sys/socket.h>
#include <net/if.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <net/if.h>

#include "rexec.h"

int
os_init(char *program, int nx)
{

	return 0;
}

int
os_pre()
{

	return 0;
}

void
os_dropcaps()
{
}

int
filter_fd(int fd, int flags, struct stat *st)
{

	return 0;
}
int
filter_load_exec(char *program, char **argv, char **envp)
{

	if (execve(program, argv, envp) == -1) {
		perror("execve");
		exit(1);
	}

	return 0;
}

int
os_emptydir()
{

	return 0;
}

int
os_extrafiles()
{
	return 0;
}

int
os_open(char *pre, char *post)
{
	struct ifreq ifr;
	memset(&ifr, 0, sizeof(ifr));

	int ifd;

	if (strcmp(pre, "tap") == 0) {
		char path[32];
		int fd, on = 1;

		sprintf(path, "/dev/%s", post);

		fd = open(path, O_RDWR | O_NONBLOCK);
		if (fd == -1) {
			fprintf(stderr, "open %s\n", path);
			return -1;
		}

		/* XXX: darwin tuntap doesn't look open arg of O_NONBLOCK */
		if (ioctl(fd, FIONBIO, &on) != 0) {
			fprintf(stderr, "ioctl(FIONBIO) %d\n", errno);
			return -1;
		}

		/* make the tap up! */
		ifd = socket(AF_INET, SOCK_DGRAM, 0);
		if (ifd < 0) {
			fprintf(stderr, "socket() for ioctl() failed\n");
			perror("socket()");
			return -1;
		}
		strncpy(ifr.ifr_name, post, IF_NAMESIZE - 1);

		if (ioctl(ifd, SIOCGIFFLAGS, &ifr) < 0) {
			fprintf(stderr, "ioctl(SIOCGIFFLAGS) %d\n", errno);
			return -1;
		}

		ifr.ifr_flags |= IFF_UP | IFF_RUNNING;

		if (ioctl(ifd, SIOCSIFFLAGS, &ifr) < 0) {
			fprintf(stderr, "ioctl(SIOCSIFFLAGS) %d\n", errno);
			return -1;
		}
		close(ifd);

		return fd;
	}

	fprintf(stderr, "platform does not support %s:%s\n", pre, post);
	exit(1);
}
