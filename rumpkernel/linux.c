#include "thread.h"

void *rumprun_thread_gettcb(void)
{
	return NULL;
}

void *rumprun_thread_create_withtls(int (*func)(void *), void *arg,
				    void *stack, int stack_size, void *tls)
{
	int jointable = 1;

	return create_thread("__clone", NULL,
			     (void (*)(void *))func, arg,
			     stack, stack_size, jointable);
}

void rumprun_thread_exit_withtls(void)
{
	exit_thread();
}

void *rumpuser_getcookie(void)
{
	return get_cookie();
}

void rumpuser_setcookie(void* cookie)
{
	set_cookie(NULL, cookie);
}
