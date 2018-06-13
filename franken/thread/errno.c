/* XXX: we don't understand but mach-o linker (ld) can't
   resolve uninitizlied global symbol if those are in .a */
#ifdef __APPLE__
int errno = 0;
#else
int errno;
#endif
