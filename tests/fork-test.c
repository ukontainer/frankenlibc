#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

int
main(int argc, char *argv[])
{
	int pid, ret, status;
	char buf[16] = "/";
	if (!argv[1])
		argv[1] = "hello";
	strcat(buf, argv[1]);
	const char *path = buf;
	const char *e_argv[] = {argv[1], argv[2], argv[3], argv[4], NULL};
	const char *e_envv[] = {"UMP_VERBOSE=1", "PATH=/bin", "LKL_BOOT_CMDLINE=child=5 mem=100M virtio-pci.force_legacy=1", NULL};

	printf("parent pid=%d, ppid=%d\n", getpid(), getppid());
	pid = vfork();
	if (pid < 0) {
		perror("cannot vfork");
		return -1;
	}

	if (pid == 0) {
		// child
		lkl_printf("child pid=%d, ppid=%d\n", getpid(), getppid());
		ret = execve(path, e_argv, e_envv);
		if (ret < 0)
			perror("execve error");

		/* should not reach */
		return -1;
	} else {
		// parent
		lkl_printf("parent ch pid=%d, pa pid=%d\n", pid, 0);
		/* XXX: since ld.so calls exit(main), this won't be called...  */
		if ((ret = waitid(P_PID, pid, NULL, WEXITED)) < 0) {
			lkl_perror("wait error", errno);
			return -1;
		}
		lkl_printf("The child (pid=%d) existed with status(%d).\n",
		       pid, WEXITSTATUS(status));

	}


	return 0;
}
