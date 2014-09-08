/* adhanas */
/* release_bs.c -- PA3, demand paging */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

/*******************************************************************************
 * Name:    release_bs 
 *
 * Desc:    Releases a BS from a process so that it can be used by other 
 *          processes. All memory mappings should have been unmapped before
 *          calling this routine. 
 *
 * Params: 
 *  bs_id   - id of the BS to be released 
 *
 * Returns: SYSCALL
 *  OK      - if everything goes fine
 *  SYSERR  - otherwise (eg., process still has some mappings lefts and so on)
 ******************************************************************************/
SYSCALL 
release_bs(bsd_t bs_id) 
{
    struct pentry *pptr = NULL;
    bs_map_t *bsptr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (FALSE == BS_IS_ID_VALID(bs_id)) {
        DTRACE("DBG$ %d %s> bad bs id %d\n", currpid, __func__, bs_id);
        goto RESTORE_AND_RETURN_ERROR;
    }

    pptr = P_GET_PPTR(currpid);
    bsptr = bsm_get_ptr_for_pid(bs_id, currpid);

    if (TRUE == BS_IS_VHEAP(bs_id)) {
        if (currpid == bsptr->bsm_pid) {
            /* The BS is a vheap of the currpid. Check if the proc has an 
             * explicit mapping to vaddr range of the BS. If so, return error.
             */
            if ((BS_VHEAP != bsptr->bsm_status) || 
                    (BS_UNMAPPED != bsptr->bsm_status)) {
                DTRACE("DBG$ %d %s> bs id %d is a vheap of currpid %d, but "    \
                        "currpid has an active mapping to this bs id\n",        \
                        currpid, __func__, bs_id, currpid);
                goto RESTORE_AND_RETURN_ERROR;
            }
            if (SYSERR == xmunmap(VPAGE_TO_VADDR(bsptr->bsm_vpno))) {
                DTRACE("DBG$ %d %s> xmunmap() failed for vheap pid %d, "        \
                        "bs id %d, vpage %d, vaadr 0x%08x\n",                   \
                         currpid, __func__, currpid, bs_id, bsptr->bsm_vpno,
                         VPAGE_TO_VADDR(bsptr->bsm_vpno));
                goto RESTORE_AND_RETURN_ERROR;
            } else {
                DTRACE("DBG$ %d %s> vheap removed for pid %d, "                 \
                        "bs id %d, vpage %d, vaadr 0x%08x\n",                   \
                         currpid, __func__, currpid, bs_id, bsptr->bsm_vpno,
                         VPAGE_TO_VADDR(bsptr->bsm_vpno));
                DTRACE("DBG$ %d %s> bs id %d state set to BS_FREE\n",           \
                        currpid, __func__, bs_id);
                bsm_tab[bs_id]->bsm_status = BS_FREE;
                pptr->bs[bs_id] = BS_FREE;
                goto RESTORE_AND_RETURN_OK;
            }
            
        } else {
            DTRACE("DBG$ %d %s> bs id %d is a vheap of pid %d, but "            \
                    "currpid %d is trying to release it\n",                     \
                    currpid, __func__, bs_id, bsptr->bsm_pid);
            goto RESTORE_AND_RETURN_ERROR;
        }
    }

    switch (pptr->bs[bs_id]) {
        case BS_FREE:
        case BS_VHEAP:
        case BS_MAPPED:
            /* Invalid BS states when calling release_bs(). Error out. */
            DTRACE("DBG$ %d %s> pid %d is trying to release bs id %d "      \
                    "which is neither being-used nor unmapped, state %d\n", \
                    currpid, __func__, bs_id, pptr->bs[bs_id]);
            goto RESTORE_AND_RETURN_ERROR;

        case BS_INUSE:
        case BS_UNMAPPED:
            /* Valid states of BS when release_bs() is called. Free the BS if no
             * active mappings are present.
             */
            pptr->bs[bs_id] = BS_FREE;
            if (0 == BS_GET_COUNT(bs_id)) {
                bsm_tab[bs_id]->bsm_status = BS_FREE;
                bsm_tab[bs_id]->bsm_pid = EMPTY;
                bsm_tab[bs_id]->bsm_isvheap = FALSE;
                bsm_tab[bs_id]->bsm_vpno = 0;
                bsm_tab[bs_id]->bsm_npages = 0;
                bsm_tab[bs_id]->bsm_sem = 0;
                bsm_tab[bs_id]->bsm_next = NULL;
                DTRACE("DBG$ %d %s> bs id %d state changed to BS_FREE\n",       \
                        currpid, __func__, bs_id);
            } else {
                DTRACE("DBG$ %d %s> release_bs() by pid %d on bs id %d is "     \
                        "valid, but there are other mappings to the bs.. not "  \
                        "chaning the state to BS_FREE\n",                       \
                        currpid, __func__, currpid, bs_id);
            }
            goto RESTORE_AND_RETURN_OK;

        default:
            goto RESTORE_AND_RETURN_OK;
    }

RESTORE_AND_RETURN_OK:
    DTRACE("DBG$ %d %s> returning OK\n", currpid, __func__);
    DTRACE_END;
    restore(ps);
    return OK;

RESTORE_AND_RETURN_ERROR:
    DTRACE("DBG$ %d %s> returning SYSERR\n", currpid, __func__);
    DTRACE_END;
    restore(ps);
    return SYSERR;
}

