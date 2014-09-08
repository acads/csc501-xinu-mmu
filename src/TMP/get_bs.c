/* adhanas */
/* get_bs.c -- PA3, demand paging */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>


/******************************************************************************
 * Name:    get_bs
 *
 * Desc:    Used to obtain a free backing store or update an exisiting 
 *          backing store with more npages.
 *
 * Params: 
 *  bs_id   - use this backing store if it's available for currpid
 *  npages  - number of pages required
 *
 * Returns: int
 *  npages  - if a new BS is created; old value npages if existing BS is used
 *  SYSERR  - in case of an error
 ******************************************************************************/
int 
get_bs(bsd_t bs_id, unsigned int npages) 
{
    STATWORD ps;
    struct pentry *pptr = NULL;
    bs_map_t *bsptr = NULL;

    disable(ps);
    DTRACE_START;

    if (FALSE == BS_IS_ID_VALID(bs_id)) { 
        DTRACE("DBG$ %d %s> bad bs_id %d\n", currpid, __func__, bs_id);
        goto RESTORE_AND_RETURN_ERROR;
    }

    if (FALSE == BS_IS_NPAGES_VALID(npages)) {
        DTRACE("DBG$ %d %s> bad npages %d\n", currpid, __func__, npages);
        goto RESTORE_AND_RETURN_ERROR;
    }

    pptr = P_GET_PPTR(currpid);
    bsptr = BS_GET_PTR(bs_id);
    if (BS_IS_VHEAP(bs_id)) {
        /* BS id belongs to another proc's vheap. Vheaps cannot be shared. */
        DTRACE("DBG$ %d %s> bs id %d is the vheap of pid %d\n",     \
                currpid, __func__, bs_id, BS_GET_PID(bs_id));
        goto RESTORE_AND_RETURN_ERROR;
            
    }

    if (TRUE == BS_IS_FREE(bs_id)) {
        /* Given bs_id is free. Update its status and return. */
        bsptr->bsm_id = bs_id;
        bsptr->bsm_pid = currpid;
        bsptr->bsm_isvheap = FALSE;
        bsptr->bsm_status = BS_INUSE;
        bsptr->bsm_npages = npages;
        bsptr->bsm_vpno = EMPTY;            /* Caller should take care. */
        pptr->bs[bs_id] = BS_INUSE;
        DTRACE("DBG$ %d %s> returning new bs_id %d with npages %d\n",   \
                currpid, __func__, bs_id, npages);
        DTRACE_END;
        restore(ps);
        return bsptr->bsm_npages;
    } else {
        /* Given BS is also mapped to some other process. Return its size. */
        pptr->bs[bs_id] = BS_INUSE;
        DTRACE("DBG$ %d %s> returning old bs_id %d with npages %d\n",   \
                currpid, __func__, bs_id, bsptr->bsm_npages);
        DTRACE_END;
        restore(ps);
        return bsptr->bsm_npages;
    }

RESTORE_AND_RETURN_ERROR:
    DTRACE("DBG$ %d %s> returning SYSERR\n", currpid, __func__);
    DTRACE_END;
    restore(ps);
    return SYSERR;
}

