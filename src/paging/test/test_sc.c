/* adhanas */
/* test_sc.c -- PA3, demand paging, SC page replacement test case */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

void
proc_test_sc(void)
{
    int i = 0;
    int j = 0;
    int cnt = 0;
    int temp = 0;
    int PAGE0 = 0x40000;
    int vaddrs[1200];
    int maxpage = 0;
   
    maxpage = (NFRAMES - (5 + 1 + 1 + 1)); 

    srpolicy(SC);
    kprintf("Policy is %d\n\n", grpolicy());

    kprintf("acquiring bs, xmmap.. start\n");
    for (i = 0; i <= (maxpage/100); ++i) {
        if (SYSERR == get_bs(i, 100)) {
            kprintf("get_bs() failed in %s %d\n", __func__, currpid);
            return;
        }

        if (SYSERR == xmmap(PAGE0 + i * 100, i, 100)) {
            kprintf("xmmap() failed in %s %d\n", __func__, currpid);
            return;
        }

        for (j = 0; j < 100; ++j) {
            vaddrs[cnt++] = (PAGE0 + (i * 100) + j) << 12;
        }
    }
    kprintf("acquiring bs, xmmap.. done\n\n");

    kprintf("initializing vaddrs.. start\n");
    print_frm_data();
    for(i = 0; i < maxpage; i++) {  
        *((int *) vaddrs[i]) = i + 1; 
    }
    print_frm_data();
    kprintf("i=%d\n", i);
    kprintf("initializing vaddrs.. done\n\n");


    //trigger page replacement, this should clear all access bits of all pages
    //expected output: frame 1032 will be swapped out
    kprintf("trigger page replacement.. start\n");
    *((int *) vaddrs[maxpage]) = maxpage + 1; 
    kprintf("trigger page repelacement.. done\n\n\n\n");

    for(i = 1; i <= maxpage; i++) {
        if (i != 500)  //reset access bits of all pages except this 
            *((int *) vaddrs[i])= i + 1;
    }

    //Expected page to be swapped: 1032+500 = 1532
    kprintf("trigger page replacement.. start\n");
    *((int *) vaddrs[maxpage+1]) = maxpage + 2; 
    temp = *((int *) vaddrs[maxpage+1]);
    kprintf("trigger page repelacement.. done\n\n");


    kprintf("xmunmap, release_bs.. start\n");
    for (i = 0; (i <= (maxpage/100)); i++) {
        xmunmap(PAGE0 + (i * 100));
        release_bs(i);
    }
    kprintf("xmunmap, release_bs.. done\n\n");

    print_frm_data();

    return;
}


int 
main(void)
{
    int pid_sc = 0;

    pid_sc = create(proc_test_sc, 1024, 20, "est_proc_sc", 0, 0);
    resume(pid_sc);

    sleep(18);
    return 0;
}
