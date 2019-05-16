#include "syscall.h"

int __platform_sigreturn(void *uc, int c, uintptr_t p)
{
	return syscall_3(SYS_sigreturn, uc, c, p);
}
