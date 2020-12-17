#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, const char **argv) {
  printf("hello2!\n");
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
