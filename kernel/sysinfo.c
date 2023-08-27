#include "types.h"
#include "param.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"
#include "defs.h"

int sysinfo(struct sysinfo *info) {
    struct proc *p = myproc();
    uint64 fmem = getfreemembybyte();
    uint64 procs = getrunningproc();

    if (copyout(p->pagetable, (uint64)&(info->freemem), (char *)&fmem, sizeof(fmem)) < 0)
        return -1;

    if (copyout(p->pagetable, (uint64)&(info->nproc), (char *)&procs, sizeof(procs)) < 0)
        return -1;

    return 0;
}