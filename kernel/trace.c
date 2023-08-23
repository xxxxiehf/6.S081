#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

int trace(int arg) {
    if (arg < 0)
        return -1;

    struct proc *p = myproc();
    p->mask = arg;

    return 0;
}