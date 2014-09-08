/* adhanas */
/* read_bs.c -- PA3, demand paging */

#include <conf.h>
#include <kernel.h>
#include <mark.h>
#include <bufpool.h>
#include <proc.h>
#include <paging.h>

/*******************************************************************************
 * Name:    read_bs
 *
 * Desc:    Copies the contents of a page from onto the frame pointed by dst.
 *          Usually used when a frame is paged in.
 *
 * Params: 
 *  dst     - PA of the frame where BS page is to be copied to
 *  bs_id   - BS id where the frame is to be copied from
 *  page    - offset within the BS
 *
 * Returns: SYSCALL
 *  OK      - on success
 *  SYSERR  - on error
 ******************************************************************************/
SYSCALL
read_bs(char *dst, bsd_t bs_id, int page) 
{
    char *bs_paddr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (FALSE == IS_VALID_PA((unsigned) dst)) {
        DTRACE("DBG$ %d %s> bad dst pa 0x%08x\n", currpid, __func__, dst);
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

    DTRACE("DBG$ %d %s> fr pa 0x%08x\n", currpid, __func__, dst);
    DTRACE("DBG$ %d %s> bs pa 0x%08x\n", currpid, __func__, bs_paddr);
    DTRACE("DBG$ %d %s> bs value %c\n", currpid, __func__, *bs_paddr);

    bcopy((void *) bs_paddr, (void *) dst, NBPG);

    DTRACE_END;
    restore(ps);
    return OK;

RESTORE_AND_RETURN_ERROR:
   DTRACE_END;
   restore(ps);
   return SYSERR;
}
