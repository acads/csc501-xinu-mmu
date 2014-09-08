/* adhanas */
/* test_bs.c -- PA3, demand paging, BS test cases */

#ifdef DBG_ON

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

void
test_getbs_A(void)
{
    int bs_id = 5;
    int npages = 120;
    int rc = -1; 

    rc = get_bs(bs_id, npages);
    kprintf("%d %s> asked %d, got %d\n", currpid, __func__, npages, rc);
#ifdef DBG_ON
    print_bs_details(bs_id, TRUE);
#endif
    sleep(2);

    rc = release_bs(bs_id);
    kprintf("%d %s: release_bs() returned %d\n", currpid, __func__, rc);
    sleep(2);

    npages = 50; 
    rc = get_bs(bs_id, npages);
    kprintf("%d %s> asked %d, got %d\n", currpid, __func__, npages, rc);
#ifdef DBG_ON
    print_bs_details(bs_id, TRUE);
#endif
    rc = xmmap(4095, 5, 50);
    kprintf("%d %s> xmmap asked %d, got %d\n", currpid, __func__, npages, rc);
    sleep(2);

    return;
}

void
test_getbs_B(void)
{
    int bs_id = 5;
    int npages = 128;
    int rc = -1; 

    rc = get_bs(bs_id, npages);
    kprintf("%d %s> asked %d, got %d\n", currpid, __func__, npages, rc);
#ifdef DBG_ON
    print_bs_details(bs_id, TRUE);
#endif
    sleep(1);

    /*  
    rc = release_bs(bs_id);
    kprintf("%d %s: release_bs() returned %d\n", currpid, __func__, rc);
    sleep(2);
    */

    return;
}

/* Test cases for get_bs() */
int
main(void)
{
    int pidA, pidB, pidC;

    DTRACE_START;
    pidA = vreate(test_getbs_A, 2000, 0, 20, "procA", 0, 0); 
    //pidB = create(test_getbs_B, 2000, 20, "procB", 0, 0); 
    //pidC = create(test_getbs_C, 2000, 20, "procC", 0, 0);

    resume(pidA);
    //resume(pidB);

    DTRACE_END;
    return;
}

#endif
