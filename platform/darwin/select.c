#include <select.h>
#include "syscall.h"

struct timeval;

int sc_select(int nfds, fd_set *restrict readfds, fd_set *restrict writefds,
	      fd_set *restrict errorfds, struct timeval *restrict timeout)
{
	return syscall_5(SYS_select, nfds, (intptr_t)readfds,
			 (intptr_t)writefds, (intptr_t)errorfds,
			 (intptr_t)timeout);
}
