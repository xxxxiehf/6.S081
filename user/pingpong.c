#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int p[2];
  char *str = "x";
  char buf[10];
  if (pipe(p) < 0){
    fprintf(2, "pingpong: failed to create pipe\n");
    exit(1);
  }

  int pid = fork();
  if (pid == 0){
    read(p[1], buf, sizeof(buf));
    close(p[1]);
    printf("%d: received ping\n", getpid());
    write(p[0], str, 1);
    exit(0);
  } else if (pid < 0) {
    fprintf(2, "pingpong: failed to fork\n");
    exit(1);
  }
  write(p[1], str, 1);
  wait(0);
  read(p[0], buf, sizeof(buf));
  close(p[0]);
  printf("%d: received pong\n", getpid());
  exit(0);
}