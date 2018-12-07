#include <unistd.h>
#include <errno.h>
#include "syscall.h"

int close(int fd)
{
	int r = __syscall(SYS_close, fd);
	if (r == -EINTR) r = 0;
	return __syscall_ret(r);
}
