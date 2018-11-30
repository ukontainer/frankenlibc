#ifndef _LIMITS_H_
#define _LIMITS_H_

#if __SIZEOF_LONG__ == 4
#define LONG_MAX  0x7fffffffL
#define ULONG_MAX 0xffffffff
#else
#define LONG_MAX  0x7fffffffffffffffL
#define ULONG_MAX 0xffffffffffffffff
#endif

#define SSIZE_MAX LONG_MAX
#define SIZE_MAX  ULONG_MAX
#define INT_MAX  0x7fffffff

#define PATH_MAX 4096
#define NAME_MAX 255

#endif
