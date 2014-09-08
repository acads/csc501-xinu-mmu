/* xdone.c - xdone */
#include <kernel.h>
#include <stdio.h>
#ifdef DBG_ON
#include <paging.h>
#endif

/*------------------------------------------------------------------------
 * xdone  --  print system completion message as last process exits
 *------------------------------------------------------------------------
 */
int xdone()
{
    kprintf("\n\nAll user processes have completed.\n\n");
#ifdef DBG_ON
    print_frm_data();
#endif
    return(OK);
}
