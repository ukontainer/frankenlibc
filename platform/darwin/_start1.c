/* platform specific initialization before we hand over to generic code */
#include <sys/stat.h>
#include <poll.h>
#include <signal.h>
#include <ucontext.h>

#include "darwin.h"
#include "init.h"
#include "syscall.h"

/* move to a header somewhere */
int __franken_start_main(int (*)(int,char **,char **), int, char **, char **);
int sigemptyset(sigset_t *);

extern int main(int, char **, char **);

extern int __platform_npoll;
extern struct pollfd __platform_pollfd[MAXFD];


// Signal handler for when code tries to use %fs.
static void handle_sigsegv(int sig, siginfo_t *si, void *ucp) {
	ucontext_t *uc = ucp;
#ifdef __x86_64__
	unsigned char *p = (unsigned char *)uc->uc_mcontext->ss.rip;
	if (*p == 0x64) {
		// Instruction starts with 0x64, meaning it tries to access %fs. By
		// changing the first byte to 0x65, it uses %gs instead.
		*p = 0x65;
	} else if (*p == 0x65) {
		// Instruction has already been patched up, but it may well be the
		// case that this was done by another CPU core. There is nothing
		// else we can do than return and try again. This may cause us to
		// get stuck indefinitely.
	} else {
		// Segmentation violation on an instruction that does not try to
		// access %fs. Reset the handler to its default action, so that the
		// segmentation violation is rethrown.
		struct sigaction sa = {
			.sa_handler = SIG_DFL,
		};
		sigemptyset(&sa.sa_mask);
		__platform_sigaction(SIGSEGV, &sa, NULL);
	}
#endif
}


void __darwin_tls_init(void)
{
#ifdef __x86_64__
	/* https://github.com/NuxiNL/cloudabi-utils/blob/38d845bc5cc6fcf441fe0d3c2433f9298cbeb760/src/libemulator/tls.c#L30-L53 */
	// On OS X there doesn't seem to be any way to modify the %fs base.
	// Let's use %gs instead. Install a signal handler for SIGSEGV to
	// dynamically patch up instructions that access %fs.
	struct sigaction sa = {
		.sa_handler = (void *)handle_sigsegv,
		.sa_flags = SA_SIGINFO,
	};
	sigemptyset(&sa.sa_mask);
	__platform_sigaction(SIGSEGV, &sa, NULL);

	/* set dummy TLS area for ELF binary
	 * https://gist.github.com/MerryMage/f22e75d5128c07d77630ca01c4272937
	 */
	unsigned long tcb;
	__asm__("mov %0, %%rdi;" :: "m"(tcb));
	__asm__("movl $0x3000003, %eax;");
	__asm__("syscall");
#endif
}

int
_start1(int argc, char **argv)
{
	char **envp = argv + argc + 1;
	int fd;
	struct darwin_stat dst;

	/* iterate over fds to pick up the special ones */
	for (fd = 0; fd < MAXFD; fd++) {
		if (syscall_2(SYS_fstat64, fd, (intptr_t)&dst) == -1)
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
