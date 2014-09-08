/* adhanas */
/* write_bs.c -- PA3, demand paging */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <mark.h>
#include <bufpool.h>
#include <paging.h>

/*******************************************************************************
 * Name:    write_bs
 *
 * Desc:    Copies the contents of a frame onto BS. Usually used when a frame
 *          is paged out.
 *
 * Params: 
 *  src     - PA of the frame to be copied
 *  bs_id   - BS id where the frame is to be copied
 *  page    - offset within the BS
 *
 * Returns: SYSCALL
 *  OK      - on success
 *  SYSERR  - on error
 ******************************************************************************/
SYSCALL 
write_bs(char *src, bsd_t bs_id, int page) 
{
    char *bs_paddr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (FALSE == IS_VALID_PA((unsigned) src)) {
        DTRACE("DBG$ %d %s> bad src pa 0x%08x\n", currpid, __func__, src);
        goto RESTORE_AND_RETURN_ERROR;
    }

    if (FALSE == BS_IS_ID_VALID(bs_id)) {
        DTRACE("DBG$ %d %s> bad bs id %d\n", currpid, __func__, bs_id);
        goto RESTORE_AND_RETURN_ERROR;
    }

    /* A BS page offset can be no less than 0 and no more than 127 as a BS
     * could only hold 128 pages, at max.
     */
    if (FALSE == FR_IS_VALID_BSOFFSET(page)) {
        DTRACE("DBG$ %d %s> bad bs offset %d\n", currpid, __func__, page);
        goto RESTORE_AND_RETURN_ERROR;
    }

    bs_paddr = (BS_BASE + (bs_id << 19) + (page * NBPG));
    bcopy((void *) src, (void *) bs_paddr, NBPG);

    DTRACE_END;
    restore(ps);
    return OK;

RESTORE_AND_RETURN_ERROR:
   DTRACE_END;
   restore(ps);
   return SYSERR;
}

