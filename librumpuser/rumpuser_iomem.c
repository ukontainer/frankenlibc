/* ported from LKL (tools/lkl/lib/iomem.c) */
#include "rumpuser_port.h"

#include <stdint.h>
#include <sys/types.h>
#include <rump/rumpuser.h>

#include "rumpuser_int.h"

#include "iomem.h"

#define IOMEM_OFFSET_BITS		24
#define IOMEM_ADDR_MARK			0x8000000
#define MAX_IOMEM_REGIONS		(IOMEM_ADDR_MARK >> IOMEM_OFFSET_BITS)

#define IOMEM_ADDR_TO_INDEX(addr) \
	((((uintptr_t)addr & ~IOMEM_ADDR_MARK) >> IOMEM_OFFSET_BITS))
#define IOMEM_ADDR_TO_OFFSET(addr) \
	(((uintptr_t)addr) & ((1 << IOMEM_OFFSET_BITS) - 1))
#define IOMEM_INDEX_TO_ADDR(i) \
	(void *)(uintptr_t)((i << IOMEM_OFFSET_BITS) | IOMEM_ADDR_MARK)

extern char lkl_virtio_devs[];

static struct iomem_region {
	void *base;
	void *iomem_addr;
	int size;
	const struct lkl_iomem_ops *ops;
} *iomem_regions[MAX_IOMEM_REGIONS];

static struct iomem_region *find_iomem_reg(void *base)
{
	int i;

	for (i = 0; i < MAX_IOMEM_REGIONS; i++)
		if (iomem_regions[i] && iomem_regions[i]->base == base)
			return iomem_regions[i];

	return NULL;
}

int register_iomem(void *base, int size, const struct lkl_iomem_ops *ops)
{
	struct iomem_region *iomem_reg;
	int i;

	if (size > (1 << IOMEM_OFFSET_BITS) - 1)
		return -1;

	if (find_iomem_reg(base))
		return -1;

	for (i = 0; i < MAX_IOMEM_REGIONS; i++)
		if (!iomem_regions[i])
			break;

	if (i >= MAX_IOMEM_REGIONS)
		return -1;

	iomem_reg = malloc(sizeof(*iomem_reg));
	if (!iomem_reg)
		return -1;

	iomem_reg->base = base;
	iomem_reg->size = size;
	iomem_reg->ops = ops;
	iomem_reg->iomem_addr = IOMEM_INDEX_TO_ADDR(i);

	iomem_regions[i] = iomem_reg;

	return 0;
}

void unregister_iomem(void *iomem_base)
{
	struct iomem_region *iomem_reg = find_iomem_reg(iomem_base);
	unsigned int index;

	if (!iomem_reg) {
		printf("%s: invalid iomem base %p\n", __func__, iomem_base);
		return;
	}

	index = IOMEM_ADDR_TO_INDEX(iomem_reg->iomem_addr);
	if (index >= MAX_IOMEM_REGIONS) {
		printf("%s: invalid iomem_addr %p\n", __func__,
			   iomem_reg->iomem_addr);
		return;
	}

	iomem_regions[index] = NULL;
	free(iomem_reg->base);
	free(iomem_reg);
}

/* rump hypercall */
void *rumpuser_ioremap(long addr, int size)
{
#ifdef GOTO_PCI
	return addr;
#else
	struct iomem_region *iomem_reg = find_iomem_reg((void *)addr);

	if (iomem_reg && size <= iomem_reg->size)
		return iomem_reg->iomem_addr;

	return NULL;
#endif
}

/* rump hypercall */
int rumpuser_iomem_access(const volatile void *addr, void *res, int size,
			  int write)
{
#ifdef GOTO_PCI			/* FIXME: should be transparent with platform/ */
	u16 mem = (unsigned long)addr;
	int ret;

	if (write) {
		if (size == 1) {
#ifdef __x86_64__
			u8 v = *(u8 *)res;
			asm volatile("outb %0, %1" :: "a"(v), "d"(mem));
#elif __arm__
#endif
			return 0;
		}
		else if (size == 2) {
#ifdef __x86_64__
			u16 v = *(u16 *)res;
			asm volatile("out %0, %1" :: "a"(v), "d"(mem));
#elif __arm__
#endif
			return 0;
		}
		else if (size == 4) {
#ifdef __x86_64__
			u32 v = *(u32 *)res;
			asm volatile("outl %0, %1" :: "a"(v), "d"(mem));
#elif __arm__
#endif
			return 0;
		}
		else if (size == 8) {
#ifdef __x86_64__
			/* XXX: not implemented yet */
			panic("not implemented yet");
#elif __arm__
#endif
			return 0;
		}
		else {
			panic("not implemented yet");
		}
	}
	else {
		if (size == 1) {
#ifdef __x86_64__
			u8 v;
			asm volatile("inb %1,%0" : "=a"(v) : "d"(mem));
			*(u8 *)res = v;
#elif __arm__
#endif
			return 0;
		}
		else if (size == 2) {
#ifdef __x86_64__
			u16 v;
			asm volatile("in %1,%0" : "=a"(v) : "d"(mem));
			*(u16 *)res = v;
#elif __arm__
#endif
			return 0;
		}
		else if (size == 4) {
#ifdef __x86_64__
			u32 v;
			asm volatile("inl %1,%0" : "=a"(v) : "d"(mem));
			*(u32 *)res = v;
#elif __arm__
#endif
			return 0;
		}
		else if (size == 8) {
#ifdef __x86_64__
			panic("not implemented yet");
#elif __arm__
#endif
		}
		else {
			panic("not implemented yet");
		}
	}
#else  /* !GOTO_PCI */
	struct iomem_region *iomem_reg;
	int index = IOMEM_ADDR_TO_INDEX(addr);
	int offset = IOMEM_ADDR_TO_OFFSET(addr);
	int ret;

	if (index > MAX_IOMEM_REGIONS || !iomem_regions[index] ||
	    offset + size > iomem_regions[index]->size)
		return -1;

	iomem_reg = iomem_regions[index];

	if (write)
		ret = iomem_reg->ops->write(iomem_reg->base, offset, res, size);
	else
		ret = iomem_reg->ops->read(iomem_reg->base, offset, res, size);
#endif
	return ret;
}

char *rumpuser_virtio_devices(void)
{
	return lkl_virtio_devs;
}
