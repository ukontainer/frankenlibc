#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

int
call_fork(const char *path, const char **e_argv, const char **e_envv)
{
	int pid, ret, status;
	siginfo_t s_info;

	printf("parent pid=%d, ppid=%d\n", getpid(), getppid());
        pid = vfork();
	if (pid < 0) {
		perror("cannot vfork");
		return -1;
	}

	if (pid == 0) {
		// child
 		printf("child pid=%d, ppid=%d\n", getpid(), getppid());
		ret = execve(path, (char *const *)e_argv,
			     (char *const *)e_envv);
		if (ret < 0)
			perror("execve error\n");

                printf("should not reach\n");
		/* should not reach */
		return -1;
	} else {
		// parent
		printf("parent ch pid=%d, pa pid=%d\n", pid, 0);
		/* XXX: since ld.so calls exit(main), this won't be called...  */
		if ((ret = waitid(P_PID, pid, NULL, WEXITED)) < 0) {
                  printf("ret: %d\n", ret);
			perror("wait error\n");
			return -1;
		}
		printf("The child (pid=%d) existed with status(%d).\n",
		       pid, WEXITSTATUS(status));

	}
        return 0;

        pid = vfork();
	if (pid < 0) {
		perror("cannot vfork");
		return -1;
	}

	if (pid == 0) {
		// child
 		printf("child pid=%d, ppid=%d\n", getpid(), getppid());
		ret = execve(path, (char *const *)e_argv,
			     (char *const *)e_envv);
		if (ret < 0)
			perror("execve error");

                printf("should not reach\n");
		/* should not reach */
		return -1;
	} else {
		// parent
		printf("parent ch pid=%d, pa pid=%d\n", pid, 0);
		/* XXX: since ld.so calls exit(main), this won't be called...  */
		if ((ret = waitid(P_PID, pid, &s_info, WEXITED)) < 0) {
			printf("ret=%d,errno=%d\n", ret, errno);
			perror("wait error");
			return -1;
		}
		printf("The child (pid=%d) existed with status(%d).\n",
		       pid, s_info.si_status);

	}

	return 0;
}

int
main(int argc, char *argv[])
{
	int ret;
	char buf[256] = "/";

	if (!argv[1])
		argv[1] = "bin/hush";
	strcat(buf, argv[1]);
	const char *path = buf;
	//const char *e_argv[] = {buf, NULL};
	const char *e_argv[5] = {buf, argv[2], argv[3], argv[4], NULL};
	const char *e_envv[4] = {"UMP_VERBOSE=1", "PATH=/bin", "LKL_BOOT_CMDLINE=child=5 mem=100M virtio-pci.force_legacy=1", NULL};

	ret = call_fork(path, e_argv, e_envv);

	return 0;
	char tmp[100];
	int i = 0;

	for (i = 0; i < 30; i++) {
		gets(tmp);
		if (!ret)
			ret =call_fork(path, e_argv, e_envv);
	}
}
