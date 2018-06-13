#ifdef __APPLE__

#define __BEGIN_DECLS
#define __END_DECLS
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE
#endif
#include <sys/ucontext.h>

#else
struct __ucontext_stack_t {
    void *ss_sp;
    int ss_flags;
    size_t ss_size;
};
typedef struct ucontext_t {
    unsigned long uc_regs[9];
    unsigned long uc_flags;
    void *uc_link;
    struct __ucontext_stack_t uc_stack;
} ucontext_t;
#endif
