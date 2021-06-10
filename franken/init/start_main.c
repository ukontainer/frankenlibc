#include <stdint.h>
#include "init.h"
#ifdef MUSL_LIBC
#include <lkl.h>
#include <lkl_config.h>
#endif


void rump_boot_setsigmodel(int);
int rump_init(void);
int rump_init_server(const char *);
int rump_pub_lwproc_rfork(int);
void rump_pub_lwproc_releaselwp(void);

#define RUMP_SIGMODEL_IGNORE 1

char **environ __attribute__((weak));
char **_environ __attribute__((weak));
char **__environ __attribute__((weak));

static char empty_string[] = "";
char *__progname = empty_string;

void _libc_init(void) __attribute__((weak));
void _libc_init() {}

void __init_libc(char **envp, char *pn);
void __libc_start_init(void);

int __franken_start_main(int (*)(int,char **,char **), int, char **, char **);

void _init(void) __attribute__ ((weak));
void _init() {}
void _fini(void) __attribute__ ((weak));
void _fini() {}

extern void (*const __init_array_start)() __attribute__((weak));
extern void (*const __init_array_end)() __attribute__((weak));
extern void (*const __fini_array_start)() __attribute__((weak));
extern void (*const __fini_array_end)() __attribute__((weak));

int atexit(void (*)(void));
void exit(int) __attribute__ ((noreturn));
static void finifn(void);

static void
finifn()
{
	uintptr_t a = (uintptr_t)&__fini_array_end;

	for (; a>(uintptr_t)&__fini_array_start; a -= sizeof(void(*)())) {
		if (*(void (**)())(a - sizeof(void(*)())) == NULL) {
			continue;
		}
		(*(void (**)())(a - sizeof(void(*)())))();
	}
	_fini();

	darwin_mod_term_func();
}

int
__franken_start_main(int(*main)(int,char **,char **), int argc, char **argv, char **envp)
{

	environ = envp;
	_environ = envp;
	__environ = envp;

	if (argv[0]) {
		char *c;
		__progname = argv[0];
		for (c = argv[0]; *c; ++c) {
			if (*c == '/')
				__progname = c + 1;
		}
	}

	__franken_fdinit();

#ifdef MUSL_LIBC
	lkl_load_config_pre(franken_lkl_get_json_config());
#endif

	rump_boot_setsigmodel(RUMP_SIGMODEL_IGNORE);
	rump_init();

#ifdef __APPLE__
	void __darwin_tls_init(void);
	__darwin_tls_init();
#endif

	/* start a new rump process */
	rump_pub_lwproc_rfork(0);

#ifndef MUSL_LIBC
	uintptr_t a;
	/* init NetBSD libc */
	_libc_init();


	_init();
	a = (uintptr_t)&__init_array_start;
	for (; a < (uintptr_t)&__init_array_end; a += sizeof(void(*)()))
		(*(void (**)())a)();
#else
	__init_libc(envp, argv[0]);
	__libc_start_init();
	darwin_mod_init_func();
#endif

	/* see if we have any devices to init */
	__franken_fdinit_create();

	/* XXX may need to have a rump kernel specific hook */
#ifdef MUSL_LIBC
	lkl_if_up(1);
	lkl_load_config_post(franken_lkl_get_json_config());
#endif

	atexit(finifn);

#ifdef MUSL_LIBC
	char *sysproxy = getenv("RUMPRUN_SYSPROXY");
	if (sysproxy) {
		if (rump_init_server(sysproxy) != 0)
			printf("failed to init sysproxy at %s\n", sysproxy);
		printf("sysproxy listening at: %s\n", sysproxy);
	}
#endif

	exit(main(argc, argv, envp));
	return 0;
}
