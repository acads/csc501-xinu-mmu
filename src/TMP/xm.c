/* adhanas */
/* xm.c -- PA3, demand paging, xmmap and xmunmap */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>
#include <stdio.h>


/*******************************************************************************
 * Name:    xmmap
 *
 * Desc:    Maps npages starting from vpage to the given BS id.
 *
 * Params: 
 *  vpage   - to be mapped from this vpage
 *  bs_id   - BS id to which the vpages should be mapped to
 *  npages  - # of vpages to map starting from vpage
 *
 * Returns: SYSCALL
 *  OK      - on success
 *  SYSERR  - on error
 ******************************************************************************/
SYSCALL 
xmmap(int vpage, bsd_t bs_id, int npages)
{
    bs_map_t *bsptr = NULL;
    struct pentry *pptr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    /* Few sanity checks. */

    /* Is bs_id within the valid range? */
    if (FALSE == BS_IS_ID_VALID(bs_id)) {
        DTRACE("DBG$ %d %s> bad bs id %d\n", currpid, __func__, bs_id);
        goto RESTORE_AND_RETURN_ERROR;
    }

    /* Is the given vpage withing the vaddr range? */
    if (FALSE == VM_IS_VPAGE_VALID(vpage)) {
        DTRACE("DBG$ %d %s> bad vpage %d\n", currpid, __func__, vpage);
        goto RESTORE_AND_RETURN_ERROR;
    }

    /* Is the given npages within the valid range? */
    if (FALSE == BS_IS_NPAGES_VALID(npages)) {
        DTRACE("DBG$ %d %s> bad npages %d\n", currpid, __func__, npages);
        goto RESTORE_AND_RETURN_ERROR;
    }

    /* We cannot map a vaddr range to a free BS. */
    if (BS_IS_FREE(bs_id)) {
        DTRACE("DBG$ %d %s> bs id %d is currently free\n",  \
                currpid, __func__, bs_id);
        goto RESTORE_AND_RETURN_ERROR;
    }

    /* We cannot map a vaddr range to a BS that is being used as a vheap. */
    if (BS_IS_VHEAP(bs_id)) {
        DTRACE("DBG$ %d %s> bs id %d is a vheap for pid %d\n",  \
                currpid, __func__, bs_id, BS_GET_PID(bs_id));
        goto RESTORE_AND_RETURN_ERROR;
    }

    /* You cannot xmmap for npages greater than what you asked for in get_bs. */
    if (npages > BS_GET_NPAGES(bs_id)) {
        DTRACE("DBG$ %d %s> asked npages %d is more than bs id %d's "   \
                "npages %d\n", \
                currpid, __func__, npages, bs_id, BS_GET_NPAGES(bs_id));
        goto RESTORE_AND_RETURN_ERROR;
    }

    /* The same pid cannot be mapped to the same BS more than once. */
    bsptr = bsm_get_ptr_for_pid(bs_id, getpid());
    if (bsptr && (getpid() == bsptr->bsm_pid) && 
            (BS_MAPPED == bsptr->bsm_status)) {
        DTRACE("DBG$ %d %s> bs id %d is already mapped to pid %d for "  \
                "vpage %d, npages %d\n",                                \
                currpid, __func__, bs_id, bsptr->bsm_pid,               \
                bsptr->bsm_vpno, bsptr->bsm_npages);
        goto RESTORE_AND_RETURN_ERROR;
    }

    if (OK != bsm_map(getpid(), vpage, bs_id, npages)) {
        DTRACE("DBG$ %d %s> bsm_map() failed for pid %d, bs id %d, "    \
                "vpage %d, npages %d\n",                                \
                currpid, __func__, currpid, bs_id, vpage, npages);
        goto RESTORE_AND_RETURN_ERROR;
    }

    pptr = P_GET_PPTR(currpid);
    pptr->bs[bs_id] = BS_MAPPED;
    DTRACE("DBG$ %d %s> xmmap() successful for pid %d, bs id %d, vaddr 0x%08x, "\
            "npages %d\n", currpid, __func__, currpid, bs_id, vpage, npages);
    DTRACE_END;
    restore(ps);
    return OK;

RESTORE_AND_RETURN_ERROR:
    DTRACE_END;
    restore(ps);
    return SYSERR;
}


/*******************************************************************************
 * Name:    xmunmap 
 *
 * Desc:    Unmaps the given vpage of the process from a BS. The given vpage
 *          could lie anywhere within the valid range of mapped pages.
 *
 * Params: 
 *  vpage   - vpage to be unmapped
 *
 * Returns: SYSCALL
 *  OK      - on success
 *  SYSERR  - on error
 ******************************************************************************/
SYSCALL 
xmunmap(int vpage)
{
    int pid = EMPTY;
    int bs_id = EMPTY;
    int offset = EMPTY;
    bs_map_t *bsptr = NULL;
    struct pentry *pptr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    pid = getpid();
    pptr = P_GET_PPTR(pid);
    if (FALSE == VM_IS_VPAGE_VALID(vpage)) {
        DTRACE("DBG$ %d %s> bad vpage %d\n", currpid, __func__, vpage);
        goto RESTORE_AND_RETURN_ERROR;
    }

    if (SYSERR == bsm_lookup(pid, VPAGE_TO_VADDR(vpage), &bs_id, &offset,
                                    NULL, NULL)) {
        DTRACE("DBG$ %d %s> bsm_lookup() failed for pid %d, vpage %d, " \
                "vaddr 0x%08x\n", currpid, __func__, currpid, vpage,    \
                VPAGE_TO_VADDR(vpage));
        goto RESTORE_AND_RETURN_ERROR;
    }

    if (BS_IS_VHEAP(bs_id)) {
        DTRACE("DBG$ %d %s> bs id %d is a vheap for pid %d.. "          \
                "cannot xmunmap is!\n",  \
                currpid, __func__, bs_id, BS_GET_PID(bs_id));
        goto RESTORE_AND_RETURN_ERROR;
    }

    /* Get the map ptr if there's one. */
    bsptr = bsm_get_ptr_for_pid(bs_id, currpid);
    if (NULL == bsptr) {
        DTRACE("DBG$ %d %s> bsm_get_ptr_for_pid() failed for pid %d, "  \
                "bs id %d, vpage %d, vaddr 0x%08x\n",                   \
                currpid, __func__, currpid, bs_id, vpage,               \
                VPAGE_TO_VADDR(vpage));
        goto RESTORE_AND_RETURN_ERROR;
    }

    /* Remove the frames associated with this vaddr range. */
    if (OK != bsm_remove_frm(currpid, bsptr)) {
        DTRACE("DBG$ %d %s> bsm_remove_frm() failed for pid %d, "       \
                "bs id %d, vpage %d, vaddr 0x%08x\n",                   \
                currpid, __func__, currpid, bs_id, vpage,               \
                VPAGE_TO_VADDR(vpage));
        goto RESTORE_AND_RETURN_ERROR;
    }

    /* Remove the mappings. */
    if (OK != bsm_unmap(pid, vpage, FALSE)) {
        DTRACE("DBG$ %d %s> bsm_unmap() failed for pid %d, vpage %d\n", \
                currpid, __func__, currpid, vpage);
        goto RESTORE_AND_RETURN_ERROR;
    }

    pptr->bs[bs_id] = BS_UNMAPPED;
    DTRACE("DBG$ %d %s> xmunmap() successful for pid %d, vpage %d\n",   \
             currpid, __func__, currpid, vpage);

    enable_paging();
    DTRACE_END;
    restore(ps);
    return OK;

RESTORE_AND_RETURN_ERROR:
    DTRACE_END;
    restore(ps);
    return SYSERR;
}

