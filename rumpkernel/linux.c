#include <stdio.h>
#include <thread.h>

void *
rumprun_thread_gettcb(void)
{
	return NULL;
}

void *
rumprun_thread_self(void)
{
	return get_current();
}

