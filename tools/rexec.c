#ifdef __Linux__
#define _GNU_SOURCE
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "rexec.h"

extern char **environ;

static void
usage(char *name)
{

	printf("Usage: %s program [-ro|-rw|-v (host):(mnt point)] [files] [-- args]\n", name);
	exit(1);
}


static char *spec_9pfs = NULL;

struct fdinfo {
	int fd;
	char name[64];
};

int fdinfo_num = 0;
struct fdinfo fdinfo_list[64];

#define FDINFO_NAME_CONFIGJSON	"__RUMP_FDINFO_CONFIGJSON"
#define FDINFO_ENV_PREFIX_NET	"__RUMP_FDINFO_NET_"
#define FDINFO_ENV_PREFIX_DISK	"__RUMP_FDINFO_DISK_"
#define FDINFO_ENV_PREFIX_ROOT	"__RUMP_FDINFO_ROOT"

/*
 * For rumpfd, Env var is __RUMP_FDINFO_NET_tap0=4 for exmaple.
 * For disk, Env var is __RUMP_FDINFO_DISK_disk.img=3 for exmaple.
 * For config.json, Env var is __RUMP_FDINFO_CONFIGJSON=2 for exmaple.
 */
void fdinfo_setenv(void)
{
	int n;
	char fdstr[8];
	for (n = 0; n < fdinfo_num; n++) {
		snprintf(fdstr, sizeof(fdstr), "%d", fdinfo_list[n].fd);
		setenv(fdinfo_list[n].name, fdstr, 1);
		fprintf(stdout, "setenv: %s=%s\n", fdinfo_list[n].name, fdstr);
	}
}


int
main(int argc, char **argv)
{
	int i, p = 0;
	int fd;
	char *program = NULL;
	char *pargs[argc];
	int mode = O_RDWR;
	int ret;
	int nfds = 3;
	int fl;
	struct stat st;
	char *path;
	char prog[4096];
	int nx = 0;
	uid_t user = 0;
	gid_t group = 0;
	int *fls;
	struct stat *stats;

	if (argc < 2)
		usage(argv[0]);

	uuid();

	for (i = 1; i < argc; i++) {
		char *arg = argv[i];

		if (strcmp(arg, "-h") == 0)
			usage(argv[0]);
		if (i == 1) {
			program = arg;
			pargs[p++] = program;
			continue;
		}
		if (strcmp(arg, "-ro") == 0) {
			mode = O_RDONLY;
			continue;
		}
		if (strcmp(arg, "-rw") == 0) {
			mode = O_RDWR;
			continue;
		}
		if (strcmp(arg, "-nx") == 0) {
			nx = 1;
			continue;
		}
		if (strcmp(arg, "-u") == 0) {
			/* XXX want to support static linking so not supporting named args because glibc */
			arg = argv[++i];
			if (i == argc) {
				fprintf(stderr, "-u needs a user id\n");
				exit(1);
			}
			user = atoi(arg);
			if (user == 0) {
				fprintf(stderr, "Will not run code as root\n");
				exit(1);
			}
			continue;
		}
		if (strcmp(arg, "-g") == 0) {
			arg = argv[++i];
			if (i == argc) {
				fprintf(stderr, "-g needs a group id\n");
				exit(1);
			}
			group = atoi(arg);
			if (group == 0) {
				fprintf(stderr, "Will not run code as wheel\n");
				exit(1);
			}
			continue;
		}
		if (strcmp(arg, "-noroot") == 0) {
			fd = open("/dev/null", O_RDONLY);
			if (fd == -1) {
				perror("open /dev/null");
				exit(1);
			}
			if (fd != 3) {
				fprintf(stderr, "-noroot must be specified before any file opened\n");
				exit(1);
			}
			continue;
		}
		if (strcmp(arg, "--") == 0) {
			i++;
			break;			
		}
		if (strcmp(arg, "-v") == 0) {
			spec_9pfs = argv[++i];
			if (i == argc) {
				fprintf(stderr, "-v needs 9pfs mount information\n");
				exit(1);
			}
			continue;
		}
		if (strchr(arg, ':')) {
			char *colon = strchr(arg, ':');
			char *pre;

			pre = strndup(arg, colon - arg);
			fd = colon_open(pre, &colon[1], mode);

			if (fd == -1) {
				free(pre);
				continue;
			}

			/* XXX:FDINFO */
			if (strcmp(pre, "config") == 0) {
				strcpy(fdinfo_list[fdinfo_num].name,
				       FDINFO_NAME_CONFIGJSON);
				fdinfo_list[fdinfo_num].fd = fd;
				fdinfo_num++;
			} else if (strcmp(pre, "tun") == 0 ||
				   strcmp(pre, "tap") == 0 ||
				   strcmp(pre, "packet") == 0 ||
				   strcmp(pre, "docker") == 0)  {

				sprintf(fdinfo_list[fdinfo_num].name, "%s%s",
					 FDINFO_ENV_PREFIX_NET, &colon[1]);
				fdinfo_list[fdinfo_num].fd = fd;
				fdinfo_num++;
			}

			free(pre);
		} else {
			fd = open(arg, mode);
			if (fd == -1) {
				fprintf(stderr, "open %s\n", strerror(errno));
				exit(1);
			}
			fl = fcntl(fd, F_GETFL);
			if (fl == -1) {
				perror("fcntl");
				exit(1);
			}
			ret = fcntl(fd, F_SETFL, fl | O_NONBLOCK);
			if (ret == -1) {
				perror("fcntl");
				exit(1);
			}

			/* XXX:FDINFO for disk */
			sprintf(fdinfo_list[fdinfo_num].name, "%s%s",
				FDINFO_ENV_PREFIX_DISK, arg);
			fdinfo_list[fdinfo_num].fd = fd;
			fdinfo_num++;
		}
	}

	for (; i < argc; i++)
		pargs[p++] = argv[i];

	pargs[p] = "\0";

	for (; p < argc; p++)
		pargs[p] = 0;

	if (strchr(program, '/') == NULL) {
		char *part;

		/* only name specified, look in PATH */
		path = getenv("PATH");
		if (! path) {
			fprintf(stderr, "no PATH set\n");
			exit(1);
		}
		path = strdup(path);
		part = strtok(path, ":");
		while (part) {
			if (strlen(part) + 1 + strlen(program) < sizeof(prog)) {
				strncpy(prog, part, sizeof(prog));
				strcat(prog, "/");
				strcat(prog, program);
				if (access(prog, R_OK | X_OK) == 0)
					break;
			}
			part = strtok(NULL, ":");
		}
		free(path);
		if (! part) {
			fprintf(stderr, "Could not find %s in PATH\n", program);
			exit(1);
		}
		program = prog;
	}

	if (stat(program, &st) == -1) {
		fprintf(stderr, "Cannot find program %s to execute\n", program);
		exit(1);
	}
	if (! S_ISREG(st.st_mode)) {
		fprintf(stderr, "Executable not a regular file\n");
		exit(1);
	}

	ret = os_init(program, nx);
	if (ret < 0) {
		fprintf(stderr, "filter_init failed\n");
		exit(1);
	}

	/* if possible cd/chroot into empty dir, but only if have some fexecve call */
	os_emptydir();

	/* if running suid, revert to original uid if none specified */
	if (user == 0 && geteuid() == 0 && getuid() != 0)
		user = getuid();
	if (user == 0 && getenv("SUDO_UID") != NULL)
		user = atoi(getenv("SUDO_UID"));
	if (group != 0) {
		if (setgid(group) == -1) {
			perror("setgid");
			exit(1);
		}
	}
	if (user != 0) {
		if (setuid(user) == -1) {
			perror("setuid");
			exit(1);
		}
	}
	os_dropcaps();
	open_9pfs();
	os_extrafiles();
	/* see how many file descriptors open */
	nfds = socket(AF_UNIX, SOCK_STREAM, 0);
	if (nfds == -1) {
		perror("socket");
		exit(1);
	}
	if (close(nfds) == -1) {
		perror("close");
		exit(1);
	}
	/* check not root */
	if (getuid() == 0 || geteuid() == 0) {
		fprintf(stderr, "Running as root user, must set -u\n");
//		exit(1);
	}
	if (setuid(0) != -1) {
		fprintf(stderr, "Can change uid to root, aborting\n");
//		exit(1);
	}
	fls = calloc(sizeof(int), nfds);
	stats = calloc(sizeof(struct stat), nfds);
	for (fd = 0; fd < nfds; fd++) {
		fl = fcntl(fd, F_GETFL);
		if (fl == -1) {
			perror("fcntl");
			exit(1);
		}
		fls[fd] = fl;
		ret = fstat(fd, &st);
		if (ret == -1) {
			perror("fstat");
			exit(1);
		}
		memcpy(&stats[fd], &st, sizeof(struct stat));
	}
	os_pre();
	for (fd = 0; fd < nfds; fd++) {
		ret = filter_fd(fd, fls[fd], &stats[fd]);
		if (ret < 0) {
			fprintf(stderr, "filter_fd failed\n");
			exit(1);
		}
	}
	free(fls);
	free(stats);

	fdinfo_setenv();

	ret = filter_load_exec(program, pargs, environ);
	if (ret < 0) {
		fprintf(stderr, "filter_load_exec failed\n");
		exit(1);
	}

	return 0;	
}

int
colon_open(char *pre, char *post, int mode)
{
	int fd, ret;

	/* open file as console for writing, use /dev/null as input */
	if (strcmp(pre, "console") == 0) {
		fd = open(post, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR);
		if (fd == -1) {
			perror("console open");
			exit(1);
		}
		ret = dup2(fd, 1);
		if (ret == -1) {
			perror("dup2");
			exit(1);
		}
		ret = dup2(fd, 2);
		if (ret == -1) {
			perror("dup2");
			exit(1);
		}
		close(fd);
		fd = open("/dev/null", O_RDONLY);
		if (ret == -1) {
			perror("open /dev/null");
			exit(1);
		}
		ret = dup2(fd, 0);
		if (ret == -1) {
			perror("dup2");
			exit(1);
		}
		close(fd);
		return -1;
	}

	if (strcmp(pre, "config") == 0) {
		fd = open(post, O_RDONLY, S_IRUSR | S_IWUSR);
		if (fd == -1) {
			fprintf(stderr, "config open error %s\n", post);
			exit(1);
		}
		return fd;
	}

	if (strcmp(pre, "rootfs") == 0) {
		int fl;

		/* disk for rootfs */
		fd = open(post, mode);
		if (fd == -1) {
			fprintf(stderr, "rootfs img open %s\n",
				strerror(errno));
			exit(1);
		}
		fl = fcntl(fd, F_GETFL);
		if (fl == -1) {
			perror("fcntl");
			exit(1);
		}
		ret = fcntl(fd, F_SETFL, fl | O_NONBLOCK);
		if (ret == -1) {
			perror("fcntl");
			exit(1);
		}

		/* XXX:FDINFO for root disk */
		sprintf(fdinfo_list[fdinfo_num].name, "%s",
			FDINFO_ENV_PREFIX_ROOT);
		fdinfo_list[fdinfo_num].fd = fd;
		fdinfo_num++;

		return fd;
	}

	fd = os_open(pre, post);
	if (fd == -1)
		exit(1);

	return fd;
}

/* create an empty working directory, chroot if able */
int
emptydir()
{
	char template[20] = "/tmp/dirXXXXXX";
	char *dir;
	int dirfd;

	dirfd = open(".", O_RDONLY|O_DIRECTORY);
	if (dirfd == -1) {
		perror("open");
		return 1;
	}

	dir = mkdtemp(template);
	if (! dir) {
		perror("mkdtemp");
		return 1;
	}

	if (chdir(dir) == -1) {
		perror("chdir");
		return 1;
	}

#ifdef AT_REMOVEDIR
	if (unlinkat(dirfd, dir, AT_REMOVEDIR) == -1) {
		perror("unlinkat");
		return 1;
	}
#endif

	if (close(dirfd) == -1) {
		perror("close");
		return 1;
	}

	if (chroot(".") == 0) {
		if (chdir("/") == -1) {
			perror("chdir");
			return 1;
		}
		if (chmod(".", 0) == -1) {
			perror("chmod");
			return -1;
		}
	}

	return 0;	
}

void
uuid()
{
	char *e = getenv("UUID");
	unsigned char rd[16];
	char uuid[37];
	char hex[3];
	int i;
	int c = 0, d = 0;
	int fd;
	ssize_t count;

	if (e) return;
	fd = open("/dev/urandom", O_RDONLY);
	if (fd == -1) {
		perror("open /dev/urandom");
		exit(1);
	}
	count = read(fd, rd, 16);
	if (count == -1) {
		perror("read /dev/urandom");
		exit(1);
	}
	if (count != 16) {
		fprintf(stderr, "short read frpom /dev/urandom");
		exit(1);
	}
	if (close(fd) == -1) {
		perror("close");
		exit(1);
	}
	for (i = 0; i < 4; i++) {
		snprintf(hex, 3, "%02x", (int)rd[c++]);
		memcpy(&uuid[d], hex, 2);
		d += 2;
	}
	uuid[d++] = '-';
	for (i = 0; i < 2; i++) {
		snprintf(hex, 3, "%02x", (int)rd[c++]);
		memcpy(&uuid[d], hex, 2);
		d += 2;
	}
	uuid[d++] = '-';
	for (i = 0; i < 2; i++) {
		snprintf(hex, 3, "%02x", (int)rd[c++]);
		memcpy(&uuid[d], hex, 2);
		d += 2;
	}
	uuid[d++] = '-';
	for (i = 0; i < 2; i++) {
		snprintf(hex, 3, "%02x", (int)rd[c++]);
		memcpy(&uuid[d], hex, 2);
		d += 2;
	}
	uuid[d++] = '-';
	for (i = 0; i < 6; i++) {
		snprintf(hex, 3, "%02x", (int)rd[c++]);
		memcpy(&uuid[d], hex, 2);
		d += 2;
	}
	uuid[d++] = 0;
	setenv("UUID", uuid, 0);
}

void
open_9pfs()
{
	if (!spec_9pfs)
		return;

	char *colon = strchr(spec_9pfs, ':');
	char *guest = &colon[1];
	char buf[16];

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror("socket");
		exit(1);
	}

	int val = 0;
	ioctl(sock, FIONBIO, &val);

	static struct sockaddr_in sin = {
		.sin_family = AF_INET,
	};
	sin.sin_port = htons(5640); /* 9pfs */
	sin.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (connect(sock, (const struct sockaddr *)&sin,
		    sizeof(sin))) {
		perror("connect");
		exit(1);
	}

	/* pass to mount point */
	setenv("9PFS_MNT", guest, 0);
	sprintf(buf, "%d", sock);
	setenv("9PFS_FD", buf, 0);

	return;
}
