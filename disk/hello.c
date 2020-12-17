#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>

int main(int argc, const char **argv) {
  int pid, ret, status;
  char buf[16] = "/";
  strcat(buf, "hello2");
  const char *path = buf;
  const char *e_argv[] = {buf, NULL};
  const char *e_envv[] = {"UMP_VERBOSE=1", "PATH=/bin", "LKL_BOOT_CMDLINE=child=5 mem=100M virtio-pci.force_legacy=1", NULL};
  printf("hello!\n");
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
		if ((ret = waitid(P_PID, pid, NULL, WEXITED)) < 0) {
                  printf("%d\n", ret);
			perror("wait error");
			return -1;
		}
		printf("The child (pid=%d) existed with status(%d).\n",
		       pid, WEXITSTATUS(status));

	}
        return 0;

  /*  printf("%d\n", argc);
  for(int i = 0; i < argc; i++) {
    printf("%s\n", argv[i]);
  }
  
  DIR *dir;
  struct dirent *dp;
  char dirpath[] = "/bin";
  
  dir = opendir(dirpath);
  if (dir == NULL) { return 1; }
  
  dp = readdir(dir);
  while (dp != NULL) {
    printf("%s\n", dp->d_name);
    dp = readdir(dir);
  }
  
  if (dir != NULL) { closedir(dir); }*/
  
  return 0;
}
