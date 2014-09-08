/* user.c - main */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

void func1(char a)  
{
    int rc = EMPTY;
    struct pentry *pptr = &proctab[currpid];
        
    kprintf("inside vheap function..\n");

    rc = get_bs(4, 110);
    kprintf("after get_bs.. %d\n", rc);
    print_nonfree_bsm_tab(TRUE);

    rc = xmmap(8000, 4, 90);
    kprintf("after xmmap.. %d\n", rc);
    print_nonfree_bsm_tab(TRUE);

    suspend(getpid());
    rc = xmunmap(8010);
    kprintf("after xmunmap.. %d\n", rc);
    print_nonfree_bsm_tab(TRUE);

    rc = release_bs(4);
    kprintf("after release_bs.. %d\n", rc);
    print_nonfree_bsm_tab(TRUE);

    rc = xmunmap(4096);
    kprintf("after unmapping vheap bs id.. %d\n", rc);
    print_nonfree_bsm_tab(TRUE);

    return;
}

int main()
{
    int rc = EMPTY;
    int pid = 0;
         
    kprintf("bsm tab before vcreate..\n");
    print_nonfree_bsm_tab(TRUE);

    pid = vcreate(func1,2000,20,50,"func1",1,'a');
    kprintf("bsm tab after vcreate..");
    print_nonfree_bsm_tab(TRUE);

    rc = get_bs(4,100);
    kprintf("after get_bs.. %d", rc);
    print_nonfree_bsm_tab(TRUE);

    rc = xmmap(7000,4,100);
    kprintf("after xmmap.. %d", rc);
    print_nonfree_bsm_tab(TRUE);

    resume(pid);
    rc = xmunmap(7000);
    kprintf("after xmunmap.. %d", rc);
    print_nonfree_bsm_tab(TRUE);

    rc = release_bs(4);
    kprintf("after release_bs.. %d", rc);
    print_nonfree_bsm_tab(TRUE);

    resume(pid);
    sleep(5);
    kprintf("after killing vcreate process..");
    print_nonfree_bsm_tab(TRUE);

    return 0;
}

/*****************************************************************************/
/* user.c - main */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

void
func1(char a)  
{
    int rc = EMPTY;
    struct pentry *pptr = &proctab[currpid];

    kprintf("inside newproc\n");

    rc = get_bs(4, 100);
    kprintf("after get_bs in newporc.. %d", rc);
    print_nonfree_bsm_tab(TRUE);

    rc = xmmap(6000, 4, 80);
    kprintf("after xmmap in newporc.. %d", rc);
    print_nonfree_bsm_tab(TRUE);

    rc = xmunmap(6000);
    kprintf("after xmunmap in newporc.. %d", rc);
    print_nonfree_bsm_tab(TRUE);

    rc = release_bs(4);
    kprintf("after release_bs in newporc.. %d", rc);
    print_nonfree_bsm_tab(TRUE);

    return;
}

int 
main()
{
    int rc = EMPTY;
    int pid = EMPTY;
    
    pid = create(func1,2000,50,"func1",1,'a');

    kprintf("bsm tab initially.. %d", rc);
    print_nonfree_bsm_tab(TRUE);

    rc = get_bs(4,100);
    kprintf("after get_b is main.. %d", rc);
    print_nonfree_bsm_tab(TRUE);

    rc = xmmap(7000,4,100);
    kprintf("after xmmap in main.. %d", rc);
    print_nonfree_bsm_tab(TRUE);

    resume(pid);

    rc = xmunmap(7000);
    kprintf("after xmunmap in main.. %d", rc);
    print_nonfree_bsm_tab(TRUE);

    rc = release_bs(4);
    kprintf("after release_bs in main.. %d", rc);
    print_nonfree_bsm_tab(TRUE);

    return 0;
}

/*****************************************************************************/
