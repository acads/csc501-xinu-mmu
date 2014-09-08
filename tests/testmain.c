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
int main()
{
	char *addr = (char*)0x40000000; //1G
	bsd_t bs=1;

	int  i= ((unsigned long)addr)>>12;	// the ith page

	kprintf("\n\nHello World, Xinu lives\n\n");

	get_bs(bs, 100);

	if (xmmap(i, bs, 100) == SYSERR) {
	    kprintf("xmmap call failed\n");
	    return 0;
	}

	for (i=0; i<16; i++){
	    *addr = 'A'+i;
	    addr += NBPG;	//increment by one page each time
	}


        addr = (char*)0x40000000; //1G
        for (i=0; i<16; i++){
            kprintf("0x%08x: %c\n", addr, *addr);
            addr += NBPG;       //increment by one page each time
        }

	xmunmap(0x40000000>>12);

	return 0;
}
