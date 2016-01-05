#ifndef _LKL_LIB_IOMEM_H
#define _LKL_LIB_IOMEM_H

struct lkl_iomem_ops {
	int (*read)(void *data, int offset, void *res, int size);
	int (*write)(void *data, int offset, void *value, int size);
};

int register_iomem(void *base, int size, const struct lkl_iomem_ops *ops);
void unregister_iomem(void *iomem_base);

#endif /* _LKL_LIB_IOMEM_H */
