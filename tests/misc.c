#include <stdio.h>
#include <sys/random.h>

int
main()
{
	char buf[1];

	/* getrandom test */
	if (getrandom(buf, 1, 0) == -1) {
		perror("getrandom");
	}

	printf("getrandom returns 0x%02X\n", buf[0] & 0xFF);

	return 0;
}
