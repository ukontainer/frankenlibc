#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>

#define MAXFD 64
int printf(const char *fmt, ...);

struct __fdtable {
	int valid;
	char *mem;
	int flags;
	int seek;
	struct stat st;
	struct thread *wake;
	int mounted;
	int hasaddr;
	struct in_addr addr;
	struct in_addr netmask;
	struct in_addr broadcast;
	struct in_addr gateway;
};

extern struct __fdtable __franken_fd[MAXFD];

void __franken_fdinit(void);
void __franken_fdinit_create(void);
struct lkl_config *franken_lkl_get_json_config(void);
#ifdef __APPLE__
void darwin_mod_init_func(void);
void darwin_mod_term_func(void);
#else
static inline void darwin_mod_init_func(void) {}
static inline void darwin_mod_term_func(void) {}
#endif
