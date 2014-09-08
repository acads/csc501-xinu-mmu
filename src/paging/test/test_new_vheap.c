/* adhansa */
/* test_new_vheap.c -- PA3, demand paging, vgetmem basic test case */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

int pa_vheap_bs = EMPTY;
int pb_vheap_bs = EMPTY;

void
procA(void)
{
    int i = 0;
    int nbytes = 0;
    char *vloc = NULL;
    char *tv = NULL;
    char *a = NULL;
    char *b = NULL;
    char *c = NULL;
    char temp;

#if 0
    kprintf("vlist of pid %d in %s before allocation..\n",   \
            getpid(), __func__, nbytes);
#ifdef DBG_ON
    print_proc_vhlist(getpid());
#endif 

    nbytes = 4076;
    kprintf("doing a vgetmem() for %d in %s..\n", nbytes, __func__);
    tv = (char *) vgetmem(nbytes);
    if ((int *) SYSERR == (int *) tv) {
        kprintf("vgetmem() for %d failed in %s..\n", nbytes, __func__);
        return;
    }

    kprintf("vlist of pid %d in %s after allocating %d bytes..\n",   \
            getpid(), __func__, nbytes);
#ifdef DBG_ON
    print_proc_vhlist(getpid());
#endif 

    kprintf("vloc in %s is 0x%08x\n", __func__, (unsigned) tv);
    *tv = 9;
    kprintf("data in %s..\n", __func__);
    kprintf("0x%08x: %d\n", tv, *tv);


#ifdef DBG_ON
#endif 
    print_proc_vhlist(getpid());

    nbytes = 4066;
    kprintf("doing a vfreemem() for %d in %s..\n", nbytes, __func__);
    if ((int *) SYSERR == vfreemem(tv, nbytes)) {
        kprintf("vfreemem() for %d failed in %s..\n", nbytes, __func__);
        return;
    }
    kprintf("vlist of pid %d in %s after a hole of 20 bytes..\n",   \
            getpid(), __func__);

#ifdef DBG_ON
    print_proc_vhlist(getpid());
#endif 

    nbytes = 16;
    kprintf("doing a vgetmem() for %d in %s..\n", nbytes, __func__);
    tv = (char *) vgetmem(nbytes);
    if ((int *) SYSERR == (int *) tv) {
        kprintf("vgetmem() for %d failed in %s..\n", nbytes, __func__);
        return;
    }

    kprintf("data in %s..\n", __func__);
    kprintf("0x%08x: %d\n", tv, *tv);

    nbytes = 16;
    kprintf("doing a vfreemem() for %d in %s..\n", nbytes, __func__);
    if ((int *) SYSERR == vfreemem(tv, nbytes)) {
        kprintf("vfreemem() for %d failed in %s..\n", nbytes, __func__);
        return;
    }

    kprintf("\n----------------------------------------------------------------\n");
#endif

    nbytes = 100;
    kprintf("doing a vgetmem() for %d in %s..\n", nbytes, __func__);
    a = (char *) vgetmem(nbytes);
    if ((int *) SYSERR == (int *) a) {
        kprintf("vgetmem() for %d failed in %s..\n", nbytes, __func__);
        return;
    }
    *a = 'a';
    kprintf("a = %c\n", *a);

    nbytes = 100;
    kprintf("doing a vgetmem() for %d in %s..\n", nbytes, __func__);
    b = (char *) vgetmem(nbytes);
    if ((int *) SYSERR == (int *) b) {
        kprintf("vgetmem() for %d failed in %s..\n", nbytes, __func__);
        return;
    }
    *b = 'b';
    kprintf("b = %c\n", *a);

    *(b + 50) = 'z';
    kprintf("b+50 = %c\n", *(b + 50));

    nbytes = 100;
    kprintf("doing a vgetmem() for %d in %s..\n", nbytes, __func__);
    c = (char *) vgetmem(nbytes);
    if ((int *) SYSERR == (int *) c) {
        kprintf("vgetmem() for %d failed in %s..\n", nbytes, __func__);
        return;
    }
    *c = 'c';
    kprintf("c = %c\n", *a);

#ifdef DBG_ON
    print_proc_vhlist(getpid());
#endif 

#if 0
    nbytes = 100;
    kprintf("doing a vfreemem() at start for %d in %s..\n", nbytes, __func__);
    if ((int *) SYSERR == vfreemem(a, nbytes)) {
        kprintf("vfreemem() for %d failed in %s..\n", nbytes, __func__);
        return;
    }
#endif 
#ifdef DBG_ON
    print_proc_vhlist(getpid());
#endif

    nbytes = 50;
    kprintf("doing a vfreemem() in middle for %d in %s..\n", nbytes, __func__);
    if ((int *) SYSERR == vfreemem(b+25, nbytes)) {
        kprintf("vfreemem() for %d failed in %s..\n", nbytes, __func__);
        return;
    }
#ifdef DBG_ON
    print_proc_vhlist(getpid());
#endif 

    nbytes = 50;
    kprintf("doing a vgetmem() for another %d in %s..\n", nbytes, __func__);
    b = (char *) vgetmem(nbytes);
    if ((int *) SYSERR == (int *) b) {
        kprintf("vgetmem() for %d failed in %s..\n", nbytes, __func__);
        return;
    }
    kprintf("newb = %c\n", *b);

#ifdef DBG_ON
    print_proc_vhlist(getpid());
#endif 
#if 0
    vloc = (char *) vgetmem(10);
    if ((int *) SYSERR == (int *) vloc) {
        kprintf("vgetmem() failed in %s..\n", __func__);
        return;
    }
    kprintf("vloc in %s is 0x%08x\n", __func__, (unsigned) vloc);
    *vloc = 'a';
    kprintf("0x%08x: %c\n", vloc, *vloc);

    vloc = (char *) vgetmem(10);
    if ((int *) SYSERR == (int *) vloc) {
        kprintf("vgetmem() failed in %s..\n", __func__);
        return;
    }
    kprintf("vloc in %s is 0x%08x\n", __func__, (unsigned) vloc);
    *vloc = 'b';
    kprintf("0x%08x: %c\n", vloc, *vloc);
#endif

    sleep(5);
    return;
}


int 
main(void)
{
    int i = 0;
    int pa = 0;
    char *bsaddr = NULL;
    char *tmp = NULL;

    bsaddr = tmp = (char *) BS_BASE;
    kprintf("before newproc in %s.. \n", __func__);
    kprintf("0x%08x: %d\n", tmp, *tmp);

    pa = vcreate(procA, 1024, 1, 20, "procA", 0, 0);

    resume(pa);
    sleep(18);

    bsaddr = tmp = (char *) BS_BASE;
    kprintf("after newproc in %s.. \n", __func__);
    kprintf("0x%08x: %d\n", tmp, *tmp);
    return;
}
