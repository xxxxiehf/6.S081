// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[];  // first address after kernel.
                    // defined by kernel.ld.

struct run {
    struct run *next;
};

struct {
    struct spinlock lock;
    struct run *freelist;
    uint8 waiting;
} kmem[NCPU];

void kinit() {
    for (int i = 0; i < NCPU; i++) {
        char *name = "kmem-0";
        name[5] += i;
        initlock(&kmem[i].lock, name);
        kmem[i].waiting = 0;
    }
    freerange(end, (void *)PHYSTOP);
}

void freerange(void *pa_start, void *pa_end) {
    char *p;
    p = (char *)PGROUNDUP((uint64)pa_start);
    for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
        kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa) {
    struct run *r;

    if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
        panic("kfree");

    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run *)pa;

    // disable interrputs to get the CPU ID
    push_off();
    int cpu_id = cpuid();
    pop_off();

    acquire(&kmem[cpu_id].lock);
    r->next = kmem[cpu_id].freelist;
    kmem[cpu_id].freelist = r;
    release(&kmem[cpu_id].lock);
}

void *kalloc_cpu(int cpu_id) {
    if (kmem[cpu_id].waiting == 1) {
        return 0;
    }
    
    struct run *r;
    
    acquire(&kmem[cpu_id].lock);
    r = kmem[cpu_id].freelist;
    if (r) {
        kmem[cpu_id].freelist = r->next;
        release(&kmem[cpu_id].lock);
        return r;
    }
    kmem[cpu_id].waiting = 1;
    release(&kmem[cpu_id].lock);
    for (int i = 0; i < NCPU; i++) {
        if (i == cpu_id) {
            continue;
        }
        r = kalloc_cpu(i);
        if (r) {
            break;
        }
    }
    kmem[cpu_id].waiting = 0;

    return r;
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *kalloc(void) {
    // disable interrputs to get the CPU ID
    push_off();
    int cpu_id = cpuid();
    pop_off();

    void *r = kalloc_cpu(cpu_id);

    if (r) {
        memset((char *)r, 5, PGSIZE);
    }

    return r;
}
