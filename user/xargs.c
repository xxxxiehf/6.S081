#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

void
xargs(int argc, char *argv[])
{
  char start[100];
  int len = 0;
  char *args[MAXARG];
  for(int i = 0; i < argc; i++){
    args[i] = malloc(strlen(argv[i]) * sizeof(char));
    strcpy(args[i], argv[i]);
  }
  while(read(0, start + len, sizeof(char))){
    if(*(start + len) == '\n' || *(start + len) == 0){
      start[len] = 0;
      int pid = fork();
      if(pid == 0){
        // printf("%s\n", argv[argc - 1]);
        args[argc] = malloc((len - 1) * sizeof(char));
        strcpy(args[argc], start);
        // printf("exec: %s\n", argv[0]);
        // for(int j = 0; j <= argc; j++){
        //   printf("%s \n", args[j]);
        // }
        exec(argv[0], args);
        free(args[argc]);
        exit(0);
      }else if(pid < 0){
        fprintf(2, "xargs: cannot fork\n");
        exit(1);
      }else{
        wait(0);
      }
      memset(start, 0, sizeof(char) * 100);
      len = 0;
    }else{
      len++;
    }
  }
}

int
main(int argc, char *argv[])
{
  if(argc < 2){
    printf("Usage: xargs [args] ...\n");
    exit(1);
  }

  xargs(argc - 1, argv + 1);
  exit(0);
}