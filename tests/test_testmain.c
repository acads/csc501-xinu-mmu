/* adhanas */
/* test_testmain.c -- PA3, demand paging, testmain.c test case */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

void
procA(void)
{
    int vpage = 9000;
    int npages = 100;
    bsd_t bs = 1;
    char *addr;
    int i = 0;

    addr = (char *) (vpage * NBPG);
    get_bs(bs, npages);

    if (xmmap(vpage, bs, npages) == SYSERR) {
        kprintf("xmmap call failed\n");
        return;
    }   

    kprintf("data in %s..\n", __func__);
    addr = (char *) (vpage * NBPG) + NBPG;
    for (i = 0; i < 26; ++i) {
        kprintf("0x%08x: %c\n", addr, *addr);
        addr += NBPG;       
    }   

    xmunmap(vpage);
    return;
}

int 
main()
{
    int vpage = 5000;
    int npages = 100;
    bsd_t bs = 1;
    char *addr;
    int i = 0;
    int pA = 0;


    kprintf("\n\nHello World, Xinu lives\n\n");

    pA = create(procA, 1024, 20, "procA", 0, 0); 

    addr = (char *) (vpage * NBPG);
    get_bs(bs, npages);

    if (xmmap(vpage, bs, npages) == SYSERR) {
        kprintf("xmmap call failed\n");
        return 0;
    }   

    for (i = 0; i < 26; ++i){
        *addr = 'A' + i;
        addr += NBPG;   
    }   

    kprintf("data in %s..\n", __func__);
    addr = (char *) (vpage * NBPG);
    for (i = 0; i < 26; ++i) {
        kprintf("0x%08x: %c\n", addr, *addr);
        addr += NBPG;       
    }   

    xmunmap(vpage);
    release_bs(bs);

    resume(pA);
    sleep(10);
    
    return 0;
}
