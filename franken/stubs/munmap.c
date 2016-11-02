#include <sys/types.h>

int _munmap(void *, size_t);
int munmap(void *, size_t);

int
munmap(void *addr, size_t length)
{

	return _munmap(addr, length);
}
