#ifdef __APPLE__

#include <stdio.h>
#include <string.h>
#include <mach-o/loader.h>
#include <mach-o/getsect.h>
#include <mach-o/ldsyms.h>


typedef void (*ctor_dtor_func)(void);

static struct section_64 *get_func_section(const char *sectname)
{
	struct segment_command_64 *sgp;
	struct section_64 *sp;
	uint32_t i, j;
	struct mach_header_64 *mhp = (struct mach_header_64 *)(&_mh_execute_header);
	const char *segname = "__DATA";

	sgp = (struct segment_command_64 *)
	      ((char *)mhp + sizeof(struct mach_header_64));
	for (i = 0; i < mhp->ncmds; i++) {
		if (sgp->cmd == LC_SEGMENT_64)
			if(strncmp(sgp->segname, segname, sizeof(sgp->segname)) == 0 ||
			   mhp->filetype == MH_OBJECT){
				sp = (struct section_64 *)((char *)sgp +
							   sizeof(struct segment_command_64));
				for(j = 0; j < sgp->nsects; j++) {
					if(strncmp(sp->sectname, sectname,
						   sizeof(sp->sectname)) == 0 &&
					   strncmp(sp->segname, segname,
						   sizeof(sp->segname)) == 0)
						return sp;
					sp = (struct section_64 *)((char *)sp +
								   sizeof(struct section_64));
				}
		}
		sgp = (struct segment_command_64 *)((char *)sgp + sgp->cmdsize);
	}
	return 0;
}

void darwin_mod_init_func(void)
{
	struct section_64 *sp = get_func_section("__mod_init_func");

	if (!sp)
		return;

	if ((sp->flags & SECTION_TYPE) == S_MOD_INIT_FUNC_POINTERS) {
		ctor_dtor_func *inits = (ctor_dtor_func *)sp->addr;
		const uint32_t count = sp->size / sizeof(uintptr_t);
		for (uint32_t i=0; i < count; ++i) {
			ctor_dtor_func func = inits[i];
			func();
		}
	}
	return;
}

void darwin_mod_term_func(void)
{
	struct section_64 *sp = get_func_section("__mod_term_func");

	if (!sp)
		return;

	if ((sp->flags & SECTION_TYPE) == S_MOD_TERM_FUNC_POINTERS) {
		ctor_dtor_func *terms = (ctor_dtor_func *)sp->addr;
		const uint32_t count = sp->size / sizeof(uintptr_t);
		for (uint32_t i=0; i < count; ++i) {
			ctor_dtor_func func = terms[i];
			func();
		}
	}
	return;
}

#endif /* __APPLE__ */
