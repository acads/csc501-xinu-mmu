
/* ------------------------------TEST CASE FOR SIMPLE ADDING AND REMOVING BSM USING VCREATE AND CREATE ---------------------------------- */

/* user.c - main */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

void halt();

/*------------------------------------------------------------------------
 *  main  --  user main program
 *------------------------------------------------------------------------
 */

void func1(char a) {

      struct pentry *pptr = &proctab[currpid];
      kprintf("\n sample function");
      bsm_unmap(currpid,pptr->vhpno,0);
      free_bsm(pptr->store);
}
int main()
{
        int pid = vcreate(func1,2000,20,50,"func1",1,'a');
        kprintf("\n pritning only pid's status");
        printbsmtab();
        get_bs(4,100);
        xmmap(7000,4,100);
        kprintf("\n printing the whole status");
        printbsmtab();
        resume(pid);
        xmunmap(7000);
        release_bs(4);
        kprintf("\n whole thing taken out");
        printbsmtab();
        return 0;
}
















/* -------------------------TEST CASE FOR 2 PROCESS CREATED USING VCREATE TO SHARE A BS ---------------------------- */

* user.c - main */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

void halt();

/*------------------------------------------------------------------------
 *  main  --  user main program
 *------------------------------------------------------------------------
 */

void func1(char a) {

      struct pentry *pptr = &proctab[currpid];
      kprintf("\n sample function");
      int r =get_bs(4,100);
      kprintf("\n the value returned is %d",r);
      xmmap(6000,4,100);
      printsecbsmtab();
      xmunmap(6000);
      release_bs(4);
}
int main()
{
        int pid = create(func1,2000,50,"func1",1,'a');
        kprintf("\n pritning only pid's status");
        printbsmtab();
        get_bs(4,100);
        xmmap(7000,4,100);
        kprintf("\n printing the whole status");
        printbsmtab();
        resume(pid);
        xmunmap(7000);
        release_bs(4);
        kprintf("\n whole thing taken out");
        printbsmtab();
        return 0;
}







/* ------------------------------ test case to see the mapping in the tabulation ----------------------- */

/* user.c - main */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

void halt();

/*------------------------------------------------------------------------
 *  main  --  user main program
 *------------------------------------------------------------------------
 */

void func1(char a) {

      struct pentry *pptr = &proctab[currpid];
      kprintf("\n sample function");
      int r =get_bs(4,100);
      kprintf("\n the value returned is %d",r);
      xmmap(6000,4,80);
      printsecbsmtab();
      xmunmap(6000);
      release_bs(4);
}
int main()
{
        int pid = create(func1,2000,50,"func1",1,'a');
        kprintf("\n pritning only pid's status");
        printbsmtab();
        get_bs(4,100);
        xmmap(7000,4,100);
        kprintf("\n printing the whole status");
        printbsmtab();
        resume(pid);
        xmunmap(7000);
        release_bs(4);
        kprintf("\n whole thing taken out");
        printbsmtab();
        printsecbsmtab();
        return 0;
}







/* ------------------------------ Test Case for multiple process to same bs and getting a new entry to bsm tab when bs has been released in bsm_tab --------*/

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

void halt();

/*------------------------------------------------------------------------
 *  main  --  user main program
 *------------------------------------------------------------------------
 */

void func1(char a) {

      struct pentry *pptr = &proctab[currpid];
      kprintf("\n sample function");
      int r =get_bs(4,100);
      kprintf("\n the value returned is %d",r);
      xmmap(6000,4,80);
      printbsmtab();
      printsecbsmtab();
      sleep(1);
      xmunmap(6000);
      release_bs(4);
}
int main()
{
        int pid1 = create(func1,2000,50,"func1",1,'a');
        int pid2 = create(func1,2000,50,"func1",1,'a');
        int pid3 = create(func1,2000,50,"func1",1,'a');
        int pid4 = create(func1,2000,50,"func1",1,'a');
        kprintf("\n Status of tables");
        printbsmtab();
        printsecbsmtab();
        get_bs(4,100);
        xmmap(7000,4,100);
        resume(pid2);
        resume(pid3);
        resume(pid4);
        resume(pid1);
        xmunmap(7000);
        release_bs(4);
        kprintf("\n final status of tables");
        printbsmtab();
        printsecbsmtab();

}
