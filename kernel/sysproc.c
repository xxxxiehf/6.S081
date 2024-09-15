#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "date.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64 sys_exit(void) {
    int n;
    if (argint(0, &n) < 0)
        return -1;
    exit(n);
    return 0; // not reached
}

uint64 sys_getpid(void) { return myproc()->pid; }

uint64 sys_fork(void) { return fork(); }

uint64 sys_wait(void) {
    uint64 p;
    if (argaddr(0, &p) < 0)
        return -1;
    return wait(p);
}

uint64 sys_sbrk(void) {
    int addr;
    int n;

    if (argint(0, &n) < 0)
        return -1;

    addr = myproc()->sz;
    if (growproc(n) < 0)
        return -1;
    return addr;
}

uint64 sys_sleep(void) {
    int n;
    uint ticks0;

    if (argint(0, &n) < 0)
        return -1;
    acquire(&tickslock);
    ticks0 = ticks;
    while (ticks - ticks0 < n) {
        if (myproc()->killed) {
            release(&tickslock);
            return -1;
        }
        sleep(&ticks, &tickslock);
    }
    release(&tickslock);
    return 0;
}

#ifdef LAB_PGTBL
// three arguments:
// 1. staring va of the first user page to check
// 2. number of pages to check
// 3. a user address to a buffer to store the results into a bitmask
int sys_pgaccess(void) {
    uint64 startaddr, bufferaddr;
    int pages;
    uint64 bitmask = 0;

    if (argaddr(0, &startaddr) < 0 || argint(1, &pages) < 0 ||
        argaddr(2, &bufferaddr) < 0)
        return -1;
    
    // set an upper limit for the page that could be scanned
    // not larger than 64 because we use uint64 to store the bitmask
    if (pages > 64 || pages <= 0)
        return -1;

    for (int i = 0; i < pages; i++) {
        uint64 va = startaddr + i * PGSIZE;
        pte_t *pte = walk(myproc()->pagetable, va, 0);
        if ((*pte & PTE_V) && (*pte & PTE_A)) {
            printf("%dth accessed, va:%p, pte:%p\n", i, va, *pte);
            bitmask |= (1L << i);
            // reset the access bit
            *pte = *pte & ~PTE_A;
        }
    }

    if (copyout(myproc()->pagetable, bufferaddr, (char *)&bitmask, pages) < 0)
        return -1;

    return 0;
}
#endif

uint64 sys_kill(void) {
    int pid;

    if (argint(0, &pid) < 0)
        return -1;
    return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64 sys_uptime(void) {
    uint xticks;

    acquire(&tickslock);
    xticks = ticks;
    release(&tickslock);
    return xticks;
}
