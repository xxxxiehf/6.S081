#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// read all numbers from left neighbours, filter and write
// to right neighbours
void
filter(int index, int left_read, int right_write) {
  int buf, num;
  read(left_read, &num, sizeof(num));
  printf("prime %d\n", num);
  while (read(left_read, &buf, sizeof(buf)) > 0) {
    if (buf == 0)
      break;
    printf("i: %d, %d\n", index, buf);
    if (buf % num != 0){
      write(1, &right_write, sizeof(right_write));
    }
  }
  printf("exit filter i: %d\n", index);
}


int
main(int argc, char *argv[])
{
  int op[2];
  int readp, writep, lwritep;
  if(pipe(op) < 0){
    fprintf(2, "prime: failed to create pipe\n");
    exit(1);
  }
  readp = op[0];
  writep = op[1];
  for(int i = 0; i < 10; i++) {
    int p[2];
    // printf("i = %d\n", i);
    while (pipe(p) < 0);
    // if(pipe(p) < 0){
    //   fprintf(2, "prime: failed to create pipe\n");
    //   exit(1);
    // }
    lwritep = writep;
    writep = p[1];
    int pid = fork();
    if(pid > 0){
      if(i == 0){
        for(int j = 2; j <= 35; j++){
          write(op[1], &j, sizeof(j));
        }
        close(op[1]);
      }
    } else if(pid < 0){
      fprintf(2, "prime: failed to fork\n");
      exit(1);
    } else {
      close(lwritep);
      filter(i, readp, writep);
      exit(0);
    }
    close(readp);
    readp = p[0];
  }
  wait(0);
  exit(0);
}