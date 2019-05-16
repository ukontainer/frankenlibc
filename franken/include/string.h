#ifndef _STRING_H_
#define _STRING_H_

#include <stdlib.h>

#define strdup(a) __franken_strdup(a)

void *memcpy(void *, const void *, size_t);
void *memset(void *, int, size_t);
char *strchr(const char *, int);
int strcmp(const char *, const char *);
char *strcpy(char *, const char *);
char *strdup(const char *);
size_t strlen(const char *);
int strncmp(const char *, const char *, size_t);
char *strncpy(char *, const char *, size_t);

#endif
