#include <sys/types.h>

#define __SYSCALL_LL_E(x)				\
((union { long long ll; long l[2]; }){ .ll = x }).l[0], \
((union { long long ll; long l[2]; }){ .ll = x }).l[1]
#define __SYSCALL_LL_O(x) 0, __SYSCALL_LL_E((x))

long (__syscall)(long, ...);

#define __asm_syscall(...) do { \
	__asm__ __volatile__ ( "svc 0" \
	: "=r"(x0) : __VA_ARGS__ : "memory", "cc"); \
	return x0; \
	} while (0)

static inline long syscall_0(long n)
{
	register long x16 __asm__("x16") = n;
	register long x0 __asm__("x0");
	__asm_syscall("r"(x16));
}

static inline long syscall_1(long n, long a)
{
	register long x16 __asm__("x16") = n;
	register long x0 __asm__("x0") = a;
	__asm_syscall("r"(x16), "0"(x0));
}

static inline long syscall_2(long n, long a, long b)
{
	register long x16 __asm__("x16") = n;
	register long x0 __asm__("x0") = a;
	register long x1 __asm__("x1") = b;
	__asm_syscall("r"(x16), "0"(x0), "r"(x1));
}

static inline long syscall_3(long n, long a, long b, long c)
{
	register long x16 __asm__("x16") = n;
	register long x0 __asm__("x0") = a;
	register long x1 __asm__("x1") = b;
	register long x2 __asm__("x2") = c;
	__asm_syscall("r"(x16), "0"(x0), "r"(x1), "r"(x2));
}

static inline long syscall_4(long n, long a, long b, long c, long d)
{
	register long x16 __asm__("x16") = n;
	register long x0 __asm__("x0") = a;
	register long x1 __asm__("x1") = b;
	register long x2 __asm__("x2") = c;
	register long x3 __asm__("x3") = d;
	__asm_syscall("r"(x16), "0"(x0), "r"(x1), "r"(x2), "r"(x3));
}

static inline long syscall_5(long n, long a, long b, long c, long d, long e)
{
	register long x16 __asm__("x16") = n;
	register long x0 __asm__("x0") = a;
	register long x1 __asm__("x1") = b;
	register long x2 __asm__("x2") = c;
	register long x3 __asm__("x3") = d;
	register long x4 __asm__("x4") = e;
	__asm_syscall("r"(x16), "0"(x0), "r"(x1), "r"(x2), "r"(x3), "r"(x4));
}

static inline long syscall_6(long n, long a, long b, long c, long d, long e, long f)
{
	register long x16 __asm__("x16") = n;
	register long x0 __asm__("x0") = a;
	register long x1 __asm__("x1") = b;
	register long x2 __asm__("x2") = c;
	register long x3 __asm__("x3") = d;
	register long x4 __asm__("x4") = e;
	register long x5 __asm__("x5") = f;
	__asm_syscall("r"(x16), "0"(x0), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5));
}

