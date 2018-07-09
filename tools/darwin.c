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

extern char *spec_9pfs;
#define FSSV_PATH "/tmp/9psv"
#define FSSV_EXE "./build/server"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
static pid_t pid_9psv;

int
os_extrafiles()
{

	if (!spec_9pfs)
		return 0;

	char *colon = strchr(spec_9pfs, ':');
	char *host = strndup(spec_9pfs, colon - spec_9pfs);
	char *guest = &colon[1];
	char *fssv_argv[] = {FSSV_EXE, "-h", FSSV_PATH, host, NULL};
	char buf[16];

	unlink(FSSV_PATH);
	/* XXX: may reimplemented w/o fd (shmem instead?) */
	/* XXX: need killing child when parent terminates */
	if ((pid_9psv = fork()) < 0) {
		perror("fork");
		exit(1);
		return (-1) ;
	}
	else if (pid_9psv == 0) {
		if (execv(FSSV_EXE, fssv_argv) == -1) {
			perror("execve lib9p server");
			exit(1);
		}
		exit(0);
	}

	/* parent process */
	usleep(10*1000);
	int sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket");
		exit(1);
	}
	static struct sockaddr_un sun = {
		.sun_family = AF_LOCAL,
		.sun_len = sizeof(sun),
	};
	memcpy(&sun.sun_path, FSSV_PATH, strlen(FSSV_PATH));
	if (connect(sock, (const struct sockaddr *)&sun,
		    sizeof(sun))) {
		perror("connect");
		exit(1);
	}

	/* pass to mount point */
	setenv("9PFS_MNT", guest, 0);
	sprintf(buf, "%d", pid_9psv);
	setenv("9PFSSV_PID", buf, 0);
	sprintf(buf, "%d", sock);
	setenv("9PFS_FD", buf, 0);

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
