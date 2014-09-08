/* adhanas */
/* test_bs.c -- PA3, demand paging, BS test cases */


#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

int pidA, pidB, pidC, pid_main, cbs;

void
test_getbs_C(void)
{
    kprintf("inside %s..\n", __func__);

    print_bs_details(0, TRUE);
    print_nonfree_bsm_tab(TRUE);


    sleep(3);
    kprintf("resuming other procs");
    resume(pidA);
    resume(pidB);
    resume(pid_main);

    sleep(20);
    return;

}

void
test_getbs_A(void)
{
#if 0
    int bs_id = 5;
    int npages = 120;
    int rc = -1; 

    rc = get_bs(bs_id, npages);
    kprintf("%d %s> asked %d, got %d\n", currpid, __func__, npages, rc);
    print_bs_details(bs_id, TRUE);
    sleep(2);

    rc = release_bs(bs_id);
    kprintf("%d %s: release_bs() returned %d\n", currpid, __func__, rc);
    sleep(2);

    npages = 50; 
    rc = get_bs(bs_id, npages);
    kprintf("%d %s> asked %d, got %d\n", currpid, __func__, npages, rc);
    print_bs_details(bs_id, TRUE);

#endif

    pidC = vcreate(test_getbs_C, 2000, 30, 20, "procC", 0, 0);
    resume(pidC);

    suspend(getpid);
    kprintf("%s resumed..\n", __func__);
    if (SYSERR == get_bs(0, 10)) {
        kprintf("get_bs() failed on vheap bs id\n");
    }

    if (SYSERR == xmmap(5000, 0, 10)) {
        kprintf("xmmap() failed on vheap bs id\n");
    }

    if (SYSERR == xmunmap(4096)) {
        kprintf("xmunmap() failed on vheap bs\n");
    }

    if (SYSERR == release_bs(0)) {
        kprintf("release_bs() failed on vheap bs\n");
    }

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
    print_bs_details(bs_id, TRUE);
    sleep(1);

    /*  
    rc = release_bs(bs_id);
    kprintf("%d %s: release_bs() returned %d\n", currpid, __func__, rc);
    sleep(2);
    */

    pidC = vcreate(test_getbs_C, 2000, 30, 20, "procC", 0, 0);
    resume(pidC);

    suspend(getpid());
    kprintf("%s resumed..\n", __func__);

    return;
}


/* Test cases for get_bs() */
int
main(void)
{
    DTRACE_START;
    pid_main = getpid();

    pidA = create(test_getbs_A, 2000, 20, "procA", 0, 0); 
    //pidB = create(test_getbs_B, 2000, 20, "procB", 0, 0); 
    //pidC = create(test_getbs_C, 2000, 20, "procC", 0, 0);

    resume(pidA);
    sleep(2);
    resume(pidB);

    suspend(getpid());
    kprintf("%s resumed..\n", __func__);

    DTRACE_END;
    return 0;
}

