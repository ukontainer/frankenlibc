#include <signal.h>
#include "syscall.h"

int __platform_sigprocmask(int how, const sigset_t *restrict set,
			   sigset_t *restrict oset)
{
	return syscall_3(SYS_sigprocmask, how, (intptr_t)set, (intptr_t)oset);
}
