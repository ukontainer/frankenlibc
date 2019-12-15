#include <poll.h>
#include <signal.h>
#include <time.h>
#include "syscall.h"
#include "init.h"
#include "thread.h"

/* XXX tidy up */
extern int __platform_npoll;
extern struct pollfd __platform_pollfd[MAXFD];

int poll(struct pollfd *fds, nfds_t n, int timeout)
{
	int i, j, ret = 0;
	int64_t sec;
	long nsec;

	if (timeout == 0) {
#ifdef SYS_poll
		return syscall(SYS_poll, fds, n, timeout);
#else
        return syscall(SYS_ppoll, fds, n, timeout>=0 ?
            &((struct timespec){ .tv_sec = timeout/1000,
             .tv_nsec = timeout%1000*1000000 }) : 0, 0, _NSIG/8);
#endif
    }

	if (timeout > 0) {
		sec = timeout / 1000;
		nsec = (timeout % 1000) * (1000*1000UL);
	} else {
		sec = 10;
		nsec = 0;
	}
	clock_sleep(CLOCK_REALTIME, sec, nsec);

	for (i = 0; i < __platform_npoll; i++) {
		if (__platform_pollfd[i].revents) {
			for (j = 0; j < (int)n; j++) {
				if (__platform_pollfd[i].fd == fds[j].fd) {
					fds[j].revents =
						__platform_pollfd[i].revents;
					ret++;
				}
			}
		}
	}

	return ret;
}
