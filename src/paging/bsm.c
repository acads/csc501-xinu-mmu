/* adhanas */
/* bsm.c -- PA3, demand paging, manage the backing store mapping*/

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

/* Actual BS tables */
bs_map_t    *bsm_tab[BS_NUM];   /* actual BS tables     */
bs_data_t   bsm_data;           /* supplemental data    */   

/*******************************************************************************
 * Name:    init_bsm
 *
 * Desc:    Initializes all the BSs.
 *
 * Params:  None.
 *
 * Returns: SYSCALL
 *  OK      - on sucess.
 *  SYSERR  - on failure
 ******************************************************************************/
SYSCALL 
init_bsm(void)
{
    uint8_t bs_id = 0;
    uint8_t bs_count = NULL;
    bs_map_t *bsptr = NULL;
    bs_map_t *bstptr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;
    for (bs_id = 0; bs_id < BS_NUM; ++bs_id) {
        bsm_tab[bs_id] = (bs_map_t *) getmem(sizeof(bs_map_t));
        bsptr = BS_GET_PTR(bs_id);
        bstptr = BS_GET_TPTR(bs_id);
        bs_count = BS_GET_COUNT(bs_id);

        bsptr->bsm_id = bs_id;
        bsptr->bsm_status = BS_FREE;
        bsptr->bsm_pid = EMPTY;
        bsptr->bsm_isvheap = FALSE;
        bsptr->bsm_vpno = 0;
        bsptr->bsm_npages = 0;
        bsptr->bsm_sem = 0;
        bsptr->bsm_next = NULL;
        bsm_data.bsm_tail[bs_id] = bsptr;
        BS_SET_COUNT(bs_id, 0);
    }
    DTRACE("DBG$ %d %s> bsm_tab initialized\n", currpid, __func__);
#ifdef DBG_ON
    print_bsm_tab(TRUE);
#endif
    DTRACE_END;
    restore(ps);
    return OK;
}


/*******************************************************************************
 * Name:    get_bsm
 *
 * Desc:    Searches through the BS table and returns a free BS id. 
 *
 * Params:
 *  avail   - pointer to store the free BS id
 *
 * Returns: SYSCALL
 *  OK      - on success
 *  SYSERR  - on error or if no free BS is available
 ******************************************************************************/
SYSCALL 
get_bsm(int* avail)
{
    uint8_t bs_id = 0;
    bs_map_t *bsptr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (NULL == avail) {
        DTRACE("DBG$ %d %s> null pointer: avail\n", currpid, __func__);
        goto RESTORE_AND_RETURN_ERROR;
    }

    for (bs_id = 0; bs_id < BS_NUM; ++bs_id) {
        if BS_IS_FREE(bs_id) {
            DTRACE("DBG$ %d %s> bs id %d is free\n", currpid, __func__, bs_id);
            *avail = bs_id;
            bsptr = BS_GET_PTR(bs_id);
            bsptr->bsm_status = BS_INUSE;
            restore(ps);
            DTRACE_END;
            return OK;
        }
    }
    DTRACE("DBG$ %d %s> no free bs id is available\n", currpid, __func__);
    *avail = EMPTY;

RESTORE_AND_RETURN_ERROR:
    DTRACE("DBG$ %d %s> returning SYSERR\n", currpid, __func__);
    restore(ps);
    DTRACE_END;
    return SYSERR;
}


/*******************************************************************************
 * Name:    free_bsm
 *
 * Desc:    Frees the contents of the given backing store. 
 *
 * Params:
 *  bs_id   - BS id whose contents are to be freed.
 *
 * Returns: SYSCALL
 *  OK      - on sucess.
 *  SYSERR  - on failure
 ******************************************************************************/
SYSCALL 
free_bsm(int bs_id)
{
    int count = 0;
    bs_map_t *bsptr = NULL;
    bs_map_t *bstptr = NULL;
    bs_map_t *victim = NULL;

    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (FALSE == BS_IS_ID_VALID(bs_id)) {
        DTRACE("DBG$ %d %s> bad bs id %d\n", currpid, __func__, bs_id);
        goto RESTORE_AND_RETURN_ERROR;
    }

    if (BS_IS_FREE(bs_id)) {
        DTRACE("DBG$ %d %s> bs id %d is already free\n",    \
                currpid, __func__, bs_id);
        goto RESTORE_AND_RETURN_ERROR;
    }

    bsptr = victim = BS_GET_PTR(bs_id);
    bstptr = BS_GET_TPTR(bs_id);
    DTRACE("DBG$ %d %s> bs id %d has %d maps\n",    \
            currpid, __func__, bs_id, BS_GET_COUNT(bs_id));
    while (victim) {
        DTRACE("DBG$ %d %s> bs id %d, map for pid %d freed\n",  \
                currpid, __func__, bs_id, victim->bsm_pid);
        bsptr = victim->bsm_next;
        victim->bsm_status = BS_FREE;
        victim->bsm_pid = EMPTY;
        victim->bsm_vpno = 0;
        victim->bsm_npages = 0;
        if (count > 0) {
            freemem((void *) victim, sizeof(bs_map_t));
        }
        BS_DEC_COUNT(bs_id);
        ++count;
        victim = bsptr;
    }
    DTRACE("DBG$ %d %s> bs id %d freed\n", currpid, __func__, bs_id);
    DTRACE_END;
    restore(ps);
    return OK;

RESTORE_AND_RETURN_ERROR:
    DTRACE("DBG$ %d %s> returning SYSERR\n", currpid, __func__);
    restore(ps);
    DTRACE_END;
    return SYSERR;
}


/*******************************************************************************
 * Name:    bsm_lookup
 *
 * Desc:    For a given pid and its vaddr, this function searches thru the 
 *          BS table to get the appropriate BS and the offset for the vaddr
 *          within the BS. 
 *
 * Params: 
 *  pid     - pid of the process
 *  vaddr   - vaddr to be looked upon
 *  store   - pointer to store the BS to which the vaddr of pid belongs to
 *  pageth  - pointer to store the offset of the vaddr within store
 *  prevmap - double pointer to store the "previous" map's location
 *  ishead  - pointer to store whether the looked up node is a head node
 *
 * Returns: SYSCALL
 *  OK      - on sucess.
 *  SYSERR  - on failure
 ******************************************************************************/
SYSCALL
bsm_lookup(int pid, long vaddr, int* store, int* pageth, 
           bs_map_t **prevmap, uint8_t *ishead)
{
    int bs_id = 0;
    uint8_t count = 0;
    uint32_t vpage = 0;
    bs_map_t *prev = NULL;
    bs_map_t *bsptr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (isbadpid(pid)) {
        DTRACE("DBG$ %d %s> bad pid %d\n", currpid, __func__, pid);
        goto RESTORE_AND_RETURN_ERROR;
    }

    if (FALSE == VADDR_IS_VALID(vaddr)) {
        DTRACE("DBG$ %d %s> bad vaddr 0x%08x, vpage %d\n",  \
                currpid, __func__, vaddr, VADDR_TO_VPAGE(vaddr));
        goto RESTORE_AND_RETURN_ERROR;
    }

    if ((NULL == store) || (NULL == pageth)) {
        DTRACE("DBG$ %d %s> null pointer - store or pageth\n",  \
                currpid, __func__);
        goto RESTORE_AND_RETURN_ERROR;
    }

    vpage = VADDR_TO_VPAGE(vaddr);

    /* A shared BS can have multiple pids mapped to it. It is implemented as 
     * a linked list. Each pid will correspond to one and only one mapping. 
     * So, go over every shared BS until the given pid and vaddr matches.
     */
    for (bs_id = 0; bs_id < BS_NUM; ++bs_id) {
        count = 0;
        bsptr = BS_GET_PTR(bs_id);
        if (BS_IS_FREE(bs_id)) {
            continue;
        }

        while (bsptr) {
            if ((bsptr->bsm_pid == pid) && 
                    (BSPTR_IS_VPAGE_VALID(bsptr, vpage))) {
                /* Got our BS. Fill store and pageth with app. values. */
                *store = bs_id;
                *pageth = BSPTR_GET_OFFSET_FOR_VPAGE(bsptr, vpage);

                /* Save the previous node and the victim position only for
                 * callers who're interested.
                 */
                if ((prevmap && ishead) && (BS_GET_COUNT(bs_id) > 1)) {
                    *prevmap = prev;
                    if (0 == count) {                        
                        DTRACE("DBG$ %d %s> pos head set\n", currpid, __func__);
                        *ishead = BS_POS_HEAD;
                    } else if (BS_GET_COUNT(bs_id) == count) {
                        DTRACE("DBG$ %d %s> pos tail set\n", currpid, __func__);
                        *ishead = BS_POS_TAIL;
                    } else {
                        DTRACE("DBG$ %d %s> pos middle set\n",  \
                                currpid, __func__);
                        *ishead = BS_POS_MID;
                    }
                }

                DTRACE("DBG$ %d %s> store %d, offset %d for pid %d, "       \
                        "vaddr 0x%08x, vpage %d\n",                         \
                        currpid, __func__, *store, *pageth, pid, vaddr, vpage);
                DTRACE_END;
                restore(ps);
                return OK;
            }
            prev = bsptr;
            bsptr = bsptr->bsm_next;
            count += 1;
        }
    }
    DTRACE("DBG$ %d %s> couldn't find a mapping for pid %d, vaddr 0x%08x, " \
            "vpage %d\n", currpid, __func__, pid, vaddr, vpage);

RESTORE_AND_RETURN_ERROR:
    DTRACE("DBG$ %d %s> returning SYSERR\n", currpid, __func__);
    DTRACE_END;
    restore(ps);
    return SYSERR;
}


/*******************************************************************************
 * Name:    bsm_map
 *
 * Desc:    Adds a mapping between the given pid, vpno and BS. 
 *
 * Params: 
 *  pid     - pid of proc whose vpno has to be mapped
 *  vpno    - vpage # of the proc to be mapped
 *  bs_id   - BS id of the BS to which the vpage is mapped
 *  npages  - # of pages in BS to map
 *
 * Returns: SYSCALL
 *  OK      - on sucess.
 *  SYSERR  - on failure
 ******************************************************************************/
SYSCALL 
bsm_map(int pid, int vpno, int bs_id, int npages)
{
    bs_map_t *bsptr = NULL;
    bs_map_t *bstptr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (isbadpid(pid)) {
        DTRACE("DBG$ %d %s> bad pid %d\n", currpid, __func__, pid);
        goto RESTORE_AND_RETURN_ERROR;
    }
    if (FALSE == BS_IS_ID_VALID(bs_id)) {
        DTRACE("DBG$ %d %s> bad bs id %d\n", currpid, __func__, bs_id);
        goto RESTORE_AND_RETURN_ERROR;
    }
    if (FALSE == VM_IS_VPAGE_VALID(vpno)) {
        DTRACE("DBG$ %d %s> bad vpno %d\n", currpid, __func__, vpno);
        goto RESTORE_AND_RETURN_ERROR;
    }
    if (FALSE == BS_IS_NPAGES_VALID(npages)) {
        DTRACE("DBG$ %d %s> bad npages %d\n", currpid, __func__, npages);
        goto RESTORE_AND_RETURN_ERROR;
    }

    /* Update the BS to reflect this mapping. */
    bsptr = BS_GET_PTR(bs_id);     /* head of the map of bs_id */
    bstptr = BS_GET_TPTR(bs_id);    /* tail of the map of bs_id */

    if (BS_GET_COUNT(bs_id)) {
        /* BS already has some maps. Append this map to the end of the list. */
        DTRACE("DBG$ %d %s> more than 1 map\n", currpid, __func__);
        bstptr->bsm_next = (bs_map_t *) getmem(sizeof(bs_map_t));
        bsptr = bstptr->bsm_next;
    }

    bsptr->bsm_id = bs_id;
    bsptr->bsm_pid = pid;
    bsptr->bsm_status = BS_MAPPED;
    bsptr->bsm_npages = npages;
    bsptr->bsm_vpno = vpno;
    bsptr->bsm_next = NULL;
    bstptr = bsptr; 
    /* Update the global tail pointer for BS.*/
    bsm_data.bsm_tail[bs_id] = bstptr;

    BS_INC_COUNT(bs_id);
    DTRACE("DBG$ %d %s> bs mapping for pid %d, bs id %d, vaddr 0x%08x, "\
            "npages %d is successful\n",                                \
            currpid, __func__, pid, bs_id, vpno, npages);

    DTRACE_END;
    restore(ps);
    return OK;

RESTORE_AND_RETURN_ERROR:
    DTRACE("DBG$ %d %s> returning SYSERR\n", currpid, __func__);
    DTRACE_END;
    restore(ps);
    return SYSERR;
}


/*******************************************************************************
 * Name:    bsm_unmap
 *
 * Desc:    Unmaps the BS assoicated with the given pid and vpage. We have to
 *          search thru all BS's maps to find the actual mapping. 
 *
 * Params: 
 *  pid     - pid whose mapping is to be removed
 *  vpage   - vpage of the pid whose mapping is to be removed
 *  flag    - not used for now
 *
 * Returns: SYSCALL
 *  OK      - on sucess
 *  SYSERR  - on failure
 ******************************************************************************/
SYSCALL 
bsm_unmap(int pid, int vpage, int flag)
{
    int bs_id = EMPTY;
    int offset = EMPTY;
    uint8_t  victim_pos = 0;
    bs_map_t *prev = NULL;
    bs_map_t *victim = NULL;
    bs_map_t *bsptr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (isbadpid(pid)) {
        DTRACE("DBG$ %d %s> bad pid %d\n", currpid, __func__, pid);
        goto RESTORE_AND_RETURN_ERROR;
    }
    if (FALSE == VM_IS_VPAGE_VALID(vpage)) {
        DTRACE("DBG$ %d %s> bad vpno %d\n", currpid, __func__, vpage);
        goto RESTORE_AND_RETURN_ERROR;
    }

    if (OK != bsm_lookup(pid, VPAGE_TO_VADDR(vpage), &bs_id, &offset, 
                            &prev, &victim_pos)) {
        DTRACE("DBG$ %d %s> bsm_lookup() failed\n", currpid, __func__);
        goto RESTORE_AND_RETURN_ERROR;
    }
    DTRACE("DBG$ %d %s> pid %d, vpage %d is mapped to bs id %d\n",  \
            currpid, __func__, pid, vpage, bs_id);
#ifdef DBG_ON
    print_bs_details(bs_id, TRUE);
#endif

    /* There could be three cases:
     * 1. Only one map in BS.
     * 2. Multiple maps and the victim is at head, middle and tail.
     */
    if (1 == (BS_GET_COUNT(bs_id))) {
        DTRACE("DBG$ %d %s> only one node\n", currpid, __func__);
        victim = bsm_tab[bs_id];
    } else {
        switch (victim_pos) {
            case BS_POS_HEAD:
                DTRACE("DBG$ %d %s> multiple, victim at head\n",    \
                        currpid, __func__);
                victim = bsm_tab[bs_id];
                prev = victim;
                bsm_tab[bs_id] = victim->bsm_next;
                break;

            case BS_POS_MID:
                DTRACE("DBG$ %d %s> multiple, victim at middle\n",  \
                        currpid, __func__);
                victim = prev->bsm_next;
                prev->bsm_next = victim->bsm_next;
                break;

            case BS_POS_TAIL:
                DTRACE("DBG$ %d %s> multiple, victim at tail\n",  \
                        currpid, __func__);
                victim = prev->bsm_next;
                prev->bsm_next = victim->bsm_next;
                bsm_data.bsm_tail[bs_id] = prev;
                break;

            default:
                DTRACE("DBG$ %d %s> bad victim position %d\n",  \
                        currpid, __func__, victim_pos);
                goto RESTORE_AND_RETURN_ERROR;
        }
    }

    victim->bsm_status = BS_UNMAPPED;
    victim->bsm_pid = EMPTY;
    victim->bsm_isvheap = FALSE;
    victim->bsm_vpno = 0;
    victim->bsm_next = NULL;
    if (prev) {
        /* Free the victim if and only if there are more than one mappings
         * in the BS, on which, prev will be non-null.
         */
        DTRACE("prev present.. freeing mem\n", NULL);
        freemem((void *) victim, sizeof(bs_map_t));
    }

    BS_DEC_COUNT(bs_id);
    DTRACE("DBG$ %d %s> bs unmapping of pid %d, bs id %d, vpage %d, "   \
            "is successful\n",                                          \
            currpid, __func__, pid, bs_id, vpage);

    /* Change the state of the BS to unmapped if there are no assoc. maps. */
    if (0 == BS_GET_COUNT(bs_id)) {
        bsptr = BS_GET_PTR(bs_id);
        bsptr->bsm_status = BS_UNMAPPED;
        DTRACE("DBG$ %d %s> chaning bs id state to unmapped as "    \
                "there are no associated processes\n",              \
                currpid, __func__, pid, bs_id, vpage);
    }

#ifdef DBG_ON
    print_bs_details(bs_id, TRUE);
#endif

    DTRACE_END;
    restore(ps);
    return OK;

RESTORE_AND_RETURN_ERROR:
    DTRACE("DBG$ %d %s> returning SYSERR\n", currpid, __func__);
    DTRACE_END;
    restore(ps);
    return SYSERR;
}


/*******************************************************************************
 * Name:    bsm_remove_frm
 *
 * Desc:    Remvoes (or decrements the refcounts) of the frames that are mapped
 *          to the vaddr range in the given bs_map. Usually called for all the
 *          maps that a process holds when its killed or for a particular map
 *          when the process unmaps it.
 *
 * Params: 
 *  pid     - pid whose frames are to be removed
 *  bsptr   - bs_map whose frames are to be removed
 *
 * Returns: SYSCALL
 *  OK      - on sucess
 *  SYSERR  - on failure
 ******************************************************************************/
SYSCALL
bsm_remove_frm(int pid, bs_map_t *bsptr)
{
    int fr_id = EMPTY;
    int fr_bs = EMPTY;
    int fr_bsoffset = EMPTY;
    int bs_vpage = EMPTY;
    int bs_limit = EMPTY;
    int bs_range = EMPTY;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (isbadpid(pid)) {
        DTRACE("DBG$ %d %s> bad pid %d\n", currpid, __func__, pid);
        goto RESTORE_AND_RETURN_ERROR;
    }

    if (NULL == bsptr) {
        DTRACE("DBG$ %d %s> null bsptr\n", currpid, __func__);
        goto RESTORE_AND_RETURN_ERROR;
    }

    if (pid != bsptr->bsm_pid) {
        DTRACE("DBG$ %d %s> pid %d and bsptr pid %d are diff\n",    \
                currpid, __func__, pid, bsptr->bsm_pid);
        goto RESTORE_AND_RETURN_ERROR;
    }

    /* Fetch the vpage start, vpage end, range from the bsptr. Now, go over
     * all the frames and for each data frame whose BS id matches ours and if
     * the frame BS offset lies within the previously calculated BS range, 
     * decrement the refcount of the frame. If the refcount reaches zero,
     * free the frame.
     */
    bs_vpage = bsptr->bsm_vpno;
    bs_limit = (bs_vpage + bsptr->bsm_npages);
    bs_range = (bs_limit - bs_vpage);
    for (fr_id = 0; fr_id < FR_NFRAMES; ++fr_id) {
        if (FR_PAGE != FR_GET_TYPE(fr_id)) {
            /* We bother only about data frames, for now. */
            continue;
        }

        fr_bs = FR_GET_BS(fr_id);
        fr_bsoffset = FR_GET_BSOFFSET(fr_id);
        if ((fr_bs == bsptr->bsm_id) && (fr_bsoffset <= bs_range)) {
            /* Got our boy. Invalidate the correspodning pgt entry and decrement
             * the refcount. The dec_frm_refcount() will free the frame once the 
             * refcount reaches zero.
             */
            DTRACE("DBG$ %d %s> decrementing refcount for fr id %d, "   \
                    "bs id %d, bs offset %d, pid %d\n",                         \
                    currpid, __func__, fr_id, fr_bs, fr_bsoffset, pid);
            dec_frm_refcount(fr_id);

            /* Clear the 'pres' bit in it's page table. */
            pgt_update_for_pid(fr_id, pid);
        }
    }

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


/*******************************************************************************
 * Name:    bsm_remove_proc_maps
 *
 * Desc:    Remvoes all the BS mappings of a process. Usually called when a 
 *          process is killed.
 *
 * Params: 
 *  pid     - pid whose mapping is to be removed
 *
 * Returns: SYSCALL
 *  OK      - on sucess
 *  SYSERR  - on failure
 ******************************************************************************/
SYSCALL 
bsm_remove_proc_maps(int pid)
{
    int bs_id = 0;
    bs_map_t *bsptr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (isbadpid(pid)) {
        DTRACE("DBG$ %d %s> bad pid %d\n", currpid, __func__, pid);
        goto RESTORE_AND_RETURN_ERROR;
    }

    /* Remove the vheap, if the pid happens to have one. First remove (and write
     * back if necessary) the associated frames, remove the BS mappings and 
     * finally release the BS.
     */
    bs_id = P_GET_BS_ID(pid);
    if (BS_IS_ID_VALID(bs_id)) {
        bsptr = BS_GET_PTR(bs_id);

        /* Cleanup the frames first. */
        if (OK != bsm_remove_frm(pid, bsptr)) {
            DTRACE("DBG$ %d %s> vheap bsm_remove_frm() failed\n",     \
                    currpid, __func__);
            goto RESTORE_AND_RETURN_ERROR;
        }
        DTRACE("DBG$ %d %s> vheap bsm_remove_frm success\n",    \
                currpid, __func__);

        /* And, now the BS mapping. */
        if (OK != bsm_unmap(pid, P_GET_VPAGE(pid), TRUE)) {
            DTRACE("DBG$ %d %s> vheap bsm_unmap() failed for pid %d, "  \
                    "bs_id %d, vpage %d\n", currpid, __func__, pid);
            goto RESTORE_AND_RETURN_ERROR;
        }
        DTRACE("DBG$ %d %s> vheap bsm_unmap success\n",    \
                currpid, __func__);
        bsm_tab[bs_id]->bsm_status = BS_FREE;
        DTRACE("DBG$ %d %s> pid %d vheap bs id %d is now free\n",    \
                currpid, __func__, pid, bs_id);
    }

    /* Go over every BS and see if it has any mappings with this pid. If so,
     * first remove the associated frames and then remove the mappings.
     */
    for (bs_id = 0; bs_id < BS_NUM; ++bs_id) {
        if (BS_IS_FREE(bs_id)) {
            continue;
        }

        bsptr = BS_GET_PTR(bs_id);
        while (bsptr) {
            if (bsptr->bsm_pid == pid) {
                /* First cleanup the frames held by this map. */
                if (OK != bsm_remove_frm(pid, bsptr)) {
                    DTRACE("DBG$ %d %s> bsm_remove_frm() failed\n",     \
                            currpid, __func__);
                    goto RESTORE_AND_RETURN_ERROR;
                }
                DTRACE("DBG$ %d %s> frms cleaned for pid %d, bs id %d " \
                        "using bsm_remove_frm()\n",                     \
                            currpid, __func__, pid, bsptr->bsm_id);

                /* Now, delete all the mappings. */
                if (OK != bsm_unmap(pid, bsptr->bsm_vpno, TRUE)) {
                    DTRACE("DBG$ %d %s> map bsm_unmap() failed for pid %d, "\
                            "bs_id %d, vpage %d\n",                         \
                            currpid, __func__, pid, bs_id, BS_GET_VPAGE(bs_id));
                    goto RESTORE_AND_RETURN_ERROR;
                }
                DTRACE("DBG$ %d %s> pid %d map is unmapped\n",  \
                        currpid, __func__, pid);
            }
            bsptr = bsptr->bsm_next;
        }
    }

    DTRACE_END;
    restore(ps);
    return OK;

RESTORE_AND_RETURN_ERROR:
    DTRACE("DBG$ %d %s> returning SYSERR\n", currpid, __func__);
    DTRACE_END;
    restore(ps);
    return SYSERR;
}


/*******************************************************************************
 * Name:    bsm_get_ptr_for_pid
 *
 * Desc:    Goes over all the maps associated with a BS and returns a pointer
 *          to the map of the given pid, if available. 
 *
 * Params: 
 *  bs_id   - BS where the pid is to be searched for.
 *  pid     - pid whose map ptr is required.
 *
 * Returns:
 *  pointer to the bs map of the pid, if available
 *  NULL, otherwise 
 ******************************************************************************/
bs_map_t *
bsm_get_ptr_for_pid(int bs_id, int pid)
{
    bs_map_t *bsptr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (isbadpid(pid)) {
        DTRACE("DBG$ %d %s> bad pid %d\n", currpid, __func__, pid);
        goto RESTORE_AND_RETURN_NULL;
    }

    if (FALSE == BS_IS_ID_VALID(bs_id)) {
        DTRACE("DBG$ %d %s> bad bs id %d\n", currpid, __func__, bs_id);
        goto RESTORE_AND_RETURN_NULL;
    }

    switch (BS_GET_STATE(bs_id)) {
        case BS_FREE:
        case BS_UNMAPPED:
            /* There will not be any mappings in these states. */
            goto RESTORE_AND_RETURN_NULL;

        case BS_INUSE:
        case BS_MAPPED:
            break;

        default:
            /* Invalid BS state. */
            goto RESTORE_AND_RETURN_NULL;
    }

    bsptr = BS_GET_PTR(bs_id);
    if (BS_IS_VHEAP(bs_id)) {
        if (pid == BS_GET_PID(bs_id)) {
            /* BS is a vheap and teh given pid is same as the BS pid. 
             * Return a pointer to this map.
             */
            goto RESTORE_AND_RETURN_PTR;
        } else {
            /* BS is a vheap and the given pid doesn't match the BS pid. */
            goto RESTORE_AND_RETURN_NULL;
        }
    }

    while (bsptr) {
        if (pid == bsptr->bsm_pid) {
            /* Found a match. Return the ptr. */
            goto RESTORE_AND_RETURN_PTR;
        }
        bsptr = bsptr->bsm_next;
    }

RESTORE_AND_RETURN_NULL:
    DTRACE("DBG$ %d %s> returning NULL\n", currpid, __func__);
    DTRACE_END;
    restore(ps);
    return NULL;

RESTORE_AND_RETURN_PTR:
    DTRACE("DBG$ %d %s> returning BSPTR with pid %d\n",    \
            currpid, __func__, bsptr->bsm_pid);
    DTRACE_END;
    restore(ps);
    return bsptr;
}


/*******************************************************************************
 * Name:    bsm_handle_proc_kill 
 *
 * Desc:    Frees all the frames belonging to a process and removes the 
 *          BS mappings. 
 *
 * Params: 
 *  pid     - pid which is being killed
 *
 * Returns: SYSCALL
 *  OK      - on success
 *  SYSERR  - on error
 ******************************************************************************/
SYSCALL
bsm_handle_proc_kill(pid)
{
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    /* Release the vhlist if the proc happens to have one. */
    vh_release_vhlist(pid);

    /* Unmap all associated BSs with the pid */
    bsm_remove_proc_maps(pid);

    /* Now, remove the page directory of the process. */
    remove_pgd(pid);

    DTRACE_END;
    restore(ps);
    return OK;
}


