/**
 *
 * lkick: a launcher program of Linux ELF binary.
 *
 * currently only tested with Linux x86_64 binary.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

int
main(int argc, char *argv[])
{
	int pid, ret, status;
	char **e_argv;
	char **e_envv = environ;

	if (argc < 2) {
		fprintf(stderr, "%s: EXECUTABLE [ARGS]\n", argv[0]);
		exit(1);
	}

	e_argv = malloc((argc) * sizeof(char *));
	if (!e_argv) {
		perror("malloc");
		exit(1);
	}
	memcpy(e_argv, &argv[1], argc * sizeof(char *));
	e_argv[argc] = 0;

	pid = vfork();
	if (pid < 0) {
		perror("cannot vfork");
		return -1;
	}

	if (pid == 0) {
		// child
		ret = execve(argv[1], e_argv, (char **)e_envv);
		if (ret < 0)
			perror("execve error");

		/* should not reach */
		return -1;
	} else {
		// parent
		/* XXX: since ld.so calls exit(main), this won't be called...  */
		if ((ret = waitid(P_PID, pid, NULL, WEXITED)) < 0) {
			perror("waitid");
			return -1;
		}
	}


	return 0;
}
