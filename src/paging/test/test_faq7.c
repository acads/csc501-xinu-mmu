/* adhanas */
/* test_faq7.c -- PA3, demand paging, FAQ #7 test case */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

void
procA(void)
{
    int rc = EMPTY;
    int vpage = 6000;
    int npages = 100;
    int bs_id = 5;
    char *x;
    char temp;

    rc = get_bs(bs_id, npages);
    kprintf("get_bs in %s.. %d\n", __func__, rc);

    rc = xmmap(vpage, bs_id, npages);
    kprintf("xmmap in %s.. %d\n", rc);

    x = (char *) (vpage * NBPG);
    kprintf("%s trying to access vaddr 0x%08x\n", __func__, x);
    *x = 'Y';
    temp = *x;
    kprintf("value of temp in %s.. %c\n", __func__, temp);
    
    sleep(15);

    temp = *x;
    kprintf("value of temp in %s.. %c\n", __func__, temp);

    rc = xmunmap(vpage);
    kprintf("xmunmap in %s.. %d\n", __func__, rc);

    rc = release_bs(bs_id);
    kprintf("release_bs in %s.. %d\n", __func__, rc);
    
    return;
}


void
procB(void)
{
    int rc = EMPTY;
    int vpage = 8000;
    int npages = 100;
    int bs_id = 5;
    char *x;
    char temp;

    rc = get_bs(bs_id, npages);
    kprintf("get_bs in %s.. %d\n", __func__, rc);

    rc = xmmap(vpage, bs_id, npages);
    kprintf("xmmap in %s.. %d\n", rc);

    x = (char *) (vpage * NBPG);
    kprintf("%s trying to access vaddr 0x%08x\n", __func__, x);
    temp = *x;
    kprintf("value of temp in %s.. %c\n", __func__, temp);

    sleep(1);

    enable_paging();
    *x =  's';
    temp = *x;
    kprintf("value of temp in %s.. %c\n", __func__, temp);

    rc = xmunmap(vpage);
    kprintf("xmunmap in %s.. %d\n", __func__, rc);

    /* Try to access VM after unmapping it. The process should be killed. */
    kprintf("value of temp in %s.. %c\n", __func__, *x);
    kprintf("This line should never be printed!\n");

    rc = release_bs(bs_id);
    kprintf("release_bs in %s.. %d\n", __func__, rc);
    
    return;
}


int 
main(void)
{
    int pa = 0;
    int pb = 0;

    pa = create(procA, 1024, 20, "procA", 0, 0);
    pb = create(procB, 1024, 20, "procB", 0, 0);

    resume(pa);
    sleep(1);
    resume(pb);

    sleep(23);
    return;
}
