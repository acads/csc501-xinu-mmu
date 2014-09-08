/* adhanas */
/* test_vheap.c -- PA3, demand paging, vheap basic test case */

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
    char *vloc = NULL;
    char *tv = NULL;
    char *after = NULL;
    char temp;

    after =vloc = tv = (char *) vgetmem(20000);
    kprintf("vloc in %s is 0x%08x\n", __func__, (unsigned) vloc);
    
    for (i = 0; i < 26;++i) {
        *tv = 'A' + i;
        tv += NBPG;
    }

    kprintf("data in %s..\n", __func__);
    for (i = 0; i < 26; ++i) {
        kprintf("0x%08x: %c\n", vloc, *vloc);
        vloc += NBPG;
    }

    kill(getpid());
    return;
}

void
procB(void)
{
    int rc = -1;
    int i = 0;
    char *vptr = NULL;
    
    rc = get_bs(0, 100);
    kprintf("%s: get_bs %d\n", __func__, rc);

    rc = xmmap(16000, 0, 100);
    kprintf("%s: xmmap %d\n", __func__, rc);

    vptr = (16000 * NBPG) + NBPG;
    kprintf("data in %s..\n", __func__);
    for (i = 0; i < 26; ++i) {
        kprintf("0x%08x: %c\n", vptr, *vptr);
        vptr += NBPG;
    }

    vptr = (16000 * NBPG) + NBPG;
    kprintf("writing data in %s..\n", __func__);
    for (i = 0; i < 26; ++i) {
        *vptr = 'a' + i;
        vptr += NBPG;
    }

    vptr = (16000 * NBPG) + NBPG;
    kprintf("after writing data in %s..\n", __func__);
    for (i = 0; i < 26; ++i) {
        kprintf("0x%08x: %c\n", vptr, *vptr);
        vptr += NBPG;
    }

    rc = xmunmap(16000 + 9);
    kprintf("%s: xmunmap %d\n", __func__, rc);

    rc = release_bs(0);
    kprintf("%s: xmmap %d\n", __func__, rc);

    return;
}


void
procC(void)
{
    int i = 0;
    char *vptr = NULL;
    char temp;

    vptr = (char *) vgetmem(20000);
    kprintf("vloc in %s is 0x%08x\n", __func__, (unsigned) vptr);
    
    kprintf("data in %s..\n", __func__);
    for (i = 0; i < 26; ++i) {
        kprintf("0x%08x: %c\n", vptr, *vptr);
        vptr += NBPG;
    }

    if ((int *) SYSERR == vfreemem(vptr, 20000)) {
        kprintf("vfreemem failed in %s..\n", __func__);
    }
    return;
}

int 
main(void)
{
    int i = 0;
    int pa = 0;
    int pb = 0;
    int pc = 0;
    char *bsaddr = NULL;
    char *tmp = NULL;

    bsaddr = tmp = (char *) BS_BASE;
    kprintf("before newproc in %s.. \n", __func__);
    for (i = 0; i < 26; ++i) {
        kprintf("0x%08x: %c\n", tmp, *tmp);
        tmp += 4096;
    }

    pa = vcreate(procA, 1024, 100, 20, "procA", 0, 0);
    pb = create(procB, 1024, 20, "procB", 0, 0);

    resume(pa);
    sleep(10);
    resume(pb);
    sleep(10);
    pc = vcreate(procC, 1024, 100, 20, "procA", 0, 0);
    resume(pc);
    sleep(10);

    bsaddr = tmp = (char *) BS_BASE;
    kprintf("after newproc in %s.. \n", __func__);
    for (i = 0; i < 26; ++i) {
        kprintf("0x%08x: %c\n", tmp, *tmp);
        tmp += 4096;
    }

    return;
}
