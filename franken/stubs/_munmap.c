#include <sys/mman.h>

int _munmap(void *, size_t);

int
_munmap(void *addr, size_t length)
{

	return munmap(addr, length);
}
