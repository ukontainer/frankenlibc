#include <unistd.h>
#include "syscall.h"

int pipe(int fildes[2])
{
	return syscall_1(SYS_pipe, (long)fildes);
}
