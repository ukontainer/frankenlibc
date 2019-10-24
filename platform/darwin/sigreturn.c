#include "syscall.h"

int __platform_sigreturn(void *uc, int c, uintptr_t p)
{
	return syscall_3(SYS_sigreturn, (intptr_t)uc, c, p);
}
