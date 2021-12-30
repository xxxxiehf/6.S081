#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[]) 
{
  int i;

  if (argc != 2){
    fprintf(2, "Usage: sleep [seconds]\n");
    exit(1);
  }
  i = atoi(argv[1]);
  
  if (sleep(i) < 0) {
    fprintf(2, "sleep: failed to sleep for %d seconds\n", argv[1]);
    exit(1);
  }

  exit(0);
}