#include "syscall.h"
#include "darwin.h"
#include <sys/mman.h>

void *mmap(void *mem, size_t length, int prot, int flags, int fd, off_t offset)
{
	int m_flags = flags;
	void *ret;

	/* Apple arm64 reports error when write data to PROT_EXEC
	 * area; thus JIT code of nodejs may not work on this arch
	 */
#if defined(__APPLE__) && defined(__arm64__)
	if ((prot & PROT_WRITE) && (prot & PROT_EXEC))
		m_flags |=  MAP_JIT;
#endif

	ret = (void *)syscall_6(SYS_mmap, (intptr_t)mem, length,
				 prot, m_flags, fd, offset);

#if defined(__APPLE__) && defined(__arm64__)
	if ((prot & PROT_WRITE) && (prot & PROT_EXEC) &&
	    (ret != MAP_FAILED))
		pthread_jit_write_protect_np(0);
#endif

	return ret;
}

/* XXX: llvm w/ Xcode12 generates this even -fno-stack-protector is. */
void ___chkstk_darwin(void)
{
}
