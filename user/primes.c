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
    if(buf % num != 0){
      write(right_write, &buf, sizeof(buf));
    }
  }
}

int
main(int argc, char *argv[])
{
  int op[2];
  int readfd, writefd;
  if(pipe(op) < 0){
    fprintf(2, "prime: failed to create pipe\n");
    exit(1);
  }
  readfd = op[0];
  writefd = op[1];
  for(int j = 2; j <= 35; j++){
    write(writefd, &j, sizeof(j));
  }
  close(writefd);
  for(int i = 0; i < 11; i++){
    int p[2];
    // printf("i = %d\n", i);
    while (pipe(p) < 0);
    writefd = p[1];
    int pid = fork();
    if(pid < 0){
      fprintf(2, "prime: failed to fork\n");
      exit(1);
    }
    if(pid == 0){
      filter(i, readfd, writefd);
      exit(0);
    }
    close(readfd);
    close(writefd);
    readfd = p[0];
    wait(0);
  }
  close(readfd);
  // while(wait(0) > 0);
  exit(0);
}