#include <signal.h>
#include "syscall.h"

#include "darwin.h"

// keep in sync with BSD_KERNEL_PRIVATE value in xnu/bsd/sys/signal.h
#define SA_VALIDATE_SIGRETURN_FROM_SIGTRAMP 0x0400

/* from darwin:/usr/include/sys/signal.h */
struct __siginfo;

/* Signal vector template for Kernel user boundary */
/* union for signal handlers */
union __darwin___sigaction_u {
        void    (*__sa_handler)(int);
        void    (*__sa_sigaction)(int, struct __siginfo *,
                       void *);
};

struct  __sigaction {
        union __darwin___sigaction_u __sigaction_u;  /* signal handler */
        void    (*sa_tramp)(void *, int, int, siginfo_t *, void *);
        sigset_t sa_mask;               /* signal mask to apply */
        int     sa_flags;               /* see signal options below */
};
#define dsa_handler      __sigaction_u.__sa_handler
#define dsa_sigaction    __sigaction_u.__sa_sigaction




static int __platform_sigaction_sys(int sig, struct sigaction *restrict act,
				    struct sigaction *restrict oact)
{
	return syscall_3(SYS_sigaction, sig, (intptr_t)act, (intptr_t)oact);
}

/**
 * use sig trampoline instead of handler.
 * code is from
 * https://github.com/apple/darwin-libplatform/blob/51ce256579c9b879704c868c9cd906ae54f733b5/src/setjmp/generic/sigaction.c
 */
int __platform_sigaction(int sig, const struct sigaction * __restrict nsv,
			 struct sigaction * __restrict osv)
{
	extern void _sigtramp();
	struct __sigaction sa;
	struct __sigaction *sap;
	int ret;

	sap = (struct __sigaction *)0;
	if (nsv) {
		sa.dsa_handler = nsv->sa_handler;
		sa.sa_tramp = _sigtramp;
		sa.sa_mask = nsv->sa_mask;
		sa.sa_flags = nsv->sa_flags | SA_VALIDATE_SIGRETURN_FROM_SIGTRAMP;
		sap = &sa;
	}
	ret = __platform_sigaction_sys(sig, (struct sigaction *)sap,
				       (struct sigaction *)osv);
	return ret;
}
