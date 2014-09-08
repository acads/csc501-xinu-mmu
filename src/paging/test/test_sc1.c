/* adhanas */
/* test_sc1.c -- PA3, demand paging, more SC test case */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

void
procA(void)
{
    return;
}

int 
main()
{
    int pA = 0;
    int i = 0;
    int bs = 1;
    int npages = 25;
    int maxpage = 20;
    int vaddr[26];
    int saddr = 4096 * 4096;

    kprintf("\n\nHello World, Xinu lives\n\n");
    pA = create(procA, 1024, 20, "procA", 0, 0);

    saddr = (4096 * NBPG);
    get_bs(bs, npages);

    if (xmmap(4096, bs, npages) == SYSERR) {
        kprintf("xmmap call failed\n");
        return 0;
    }
    print_bs_details(bs, TRUE);

    for (i = 0; i < npages; ++i){
        vaddr[i] = saddr + (i * NBPG);
        kprintf("vaddr[%d] = 0x%08x\n", i, vaddr[i]);
    }

    for (i = 0; i < 20; ++i) {
        *((int *) vaddr[i]) = i * 10;
    }
    print_frm_data();

    *((int *) vaddr[20]) = 2222;

    for (i = 0; i < 20; ++i) {
        if (0 == (i % 2))
            *((int *) vaddr[i] = (8000 + i));
    }

    kprintf("data in %s..\n", __func__);
    for (i = 0; i < npages; ++i) {
        kprintf("0x%08x: %d\n", vaddr[i], *((int *) vaddr[i]));
    }

    xmunmap(saddr);
    release_bs(bs);

    //resume(pA);
    sleep(10);

    return 0;
}
