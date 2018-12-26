/* platform specific initialization before we hand over to generic code */
#include <sys/stat.h>
#include <poll.h>

#include "darwin.h"
#include "init.h"
#include "syscall.h"

/* move to a header somewhere */
int __franken_start_main(int (*)(int,char **,char **), int, char **, char **);

extern int main(int, char **, char **);

extern int __platform_npoll;
extern struct pollfd __platform_pollfd[MAXFD];

int
_start1(int argc, char **argv)
{
	char **envp = argv + argc + 1;
	int fd;
	struct darwin_stat dst;

	/* iterate over fds to pick up the special ones */
	for (fd = 0; fd < MAXFD; fd++) {
		if (syscall_2(SYS_fstat64, fd, &dst) == -1)
			break;

		switch (dst.st_mode & DARWIN_S_IFMT) {
		case DARWIN_S_IFCHR:
			/* XXX this only works fine with tuntaposx */
			if (major(dst.st_rdev) > 30) {
				/* say we are a "socket" ie network device */
				dst.st_mode = DARWIN_S_IFSOCK;
				/* add to poll */
				__platform_pollfd[__platform_npoll].fd = fd;
				__platform_pollfd[__platform_npoll].events
					= POLLIN | POLLPRI;
				__platform_npoll++;
			}
			break;
		}
	}


	return __franken_start_main(main, argc, argv, envp);
}
