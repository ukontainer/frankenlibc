#include <string.h>

void memset_pattern4(void *dst, const void *src, size_t len)
{
	while (len > 0) {
		if (len > 3)
			memcpy(dst, src, 4);
		else {
			memcpy(dst, src, len);
			return;
		}
		dst += 4;
		len -= 4;
	}
}

void memset_pattern8(void *dst, const void *src, size_t len)
{
	while (len > 0) {
		if (len > 7)
			memcpy(dst, src, 8);
		else {
			memcpy(dst, src, len);
			return;
		}
		dst += 8;
		len -= 8;
	}
}
void memset_pattern16(void *dst, const void *src, size_t len)
{
	while (len > 0) {
		if (len > 16)
			memcpy(dst, src, 16);
		else {
			memcpy(dst, src, len);
			return;
		}
		dst += 16;
		len -= 16;
	}
}
