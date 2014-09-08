/* adhanas */
/* frame.c - PA3, demang paging, manage physical frames */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

frm_map_t frm_tab[FR_NFRAMES];      /* actual frame table   */
frm_data_t frm_data;                /* frame debug data     */


/*******************************************************************************
 * Name:    init_frm_tab
 *
 * Desc:    Initializes all the frames in the system. Usually called during
 *          system boot.
 *
 * Params:  None.
 *
 * Returns: Nothing. 
 ******************************************************************************/
void
init_frm_tab(void)
{
    int frame = 0;
    frm_map_t *fptr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    bzero(&frm_data, sizeof(frm_data));
    frm_data.fr_nfree = FR_NFRAMES;
    for (frame = 0; frame < FR_NFRAMES; ++frame) {
        fptr = FR_GET_FPTR(frame);
        fptr->fr_status = FR_FREE;
        fptr->fr_pid = EMPTY;
        bzero(fptr->fr_vpage, (NPROC * sizeof(int)));
        fptr->fr_refcnt = 0;
        fptr->fr_type = FR_FREE;
        fptr->fr_dirty = FALSE;
        fptr->fr_bs = EMPTY;
        fptr->fr_bsoffset = EMPTY;
        bzero(fptr->fr_pidmap, (7 * sizeof(unsigned char)));
        //fptr->fr_cookie = NULL;
        fptr->fr_ltime = 0;
    }
    DTRACE("DBG$ %d %s> frm_tab initialized\n", currpid, __func__);
    DTRACE_END;
    restore(ps);
    return;
}


/*******************************************************************************
 * Name:    init_frm
 *
 * Desc:    Initializes the given frame. Usually called during on frame alloc.
 *
 * Params:
 *  fr_id   - pointer to the frame to be initialized.
 *
 * Returns: Nothing. 
 ******************************************************************************/
void
init_frm(int fr_id)
{
    frm_map_t *fptr = NULL;
    STATWORD ps;

    if (FALSE == FR_IS_ID_VALID(fr_id)) {
        DTRACE("DBG$ %d %s> bad fr id %d\n", currpid, __func__, fr_id);
        return;
    }

    disable(ps);
    DTRACE_START;

    fptr = FR_GET_FPTR(fr_id);
    fptr->fr_id = fr_id;
    fptr->fr_status = FR_FREE;
    fptr->fr_pid = EMPTY;
    bzero(fptr->fr_vpage, (NPROC * sizeof(int)));
    fptr->fr_refcnt = 0;
    fptr->fr_type = FR_FREE;
    fptr->fr_dirty = FALSE;
    fptr->fr_bs = EMPTY;
    fptr->fr_bsoffset = EMPTY;
    bzero(fptr->fr_pidmap, (7 * sizeof(unsigned char)));
    //fptr->fr_cookie = NULL;
    fptr->fr_ltime = 0;

    DTRACE("DBG$ %d %s> frame %d initialized\n",    \
            currpid, __func__, fptr->fr_id);
    DTRACE_END;
    restore(ps);
    return;
}

frm_map_t *
evict_frm_nru(void)
{
    return NULL;
}


/*******************************************************************************
 * Name:    frm_pidmap_oper
 *
 * Desc:    Performs operations such as set, clear and lookup on the pidmap
 *          bit-vector on the given frame. 
 *
 * Params:
 *  fr_id   - id of the frame on whose pidmap operations are to be performed
 *  pid     - pid in the pidmap
 *  oper    - set, clear or lookup
 *
 * Returns:
 *  TRUE    - always for set & clear ops; only if the pid bit is set in lookup
 *  FALSE   - applicable only for lookup op; if the pid bit is not set 
 ******************************************************************************/
int
frm_pidmap_oper(int fr_id, int pid, int oper)
{
    int bit = -1;
    int index = -1;
    char *map;
    frm_map_t *frptr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (FALSE == FR_IS_ID_VALID(fr_id)) {
        DTRACE("DBG$ %d %s> bad fr id %d\n", currpid, __func__, fr_id);
        goto RESTORE_AND_RETURN_FALSE;
    }

    if (isbadpid(pid)) {
        DTRACE("DBG$ %d %s> bad pid %d\n", currpid, __func__, pid);
        goto RESTORE_AND_RETURN_FALSE;
    }

    frptr = FR_GET_FPTR(fr_id);
    index = pid / 8;
    bit = pid % 8;
    map = &(frptr->fr_pidmap[index]);

    switch (oper) {
        case FR_OP_PMAP_SET:
            *map = (*map | (1 << bit));
            DTRACE("DBG$ %d %s> fr id %d setting index %d, bit %d for " \
                    "pid %d\n", currpid, __func__, fr_id, index, bit, pid);
            goto RESTORE_AND_RETURN_TRUE;

        case FR_OP_PMAP_CLR:
            *map = (*map & ~(1 << bit));
            DTRACE("DBG$ %d %s> fr id %d clearing index %d, bit %d for " \
                    "pid %d\n", currpid, __func__, fr_id, index, bit, pid);
            goto RESTORE_AND_RETURN_TRUE;

        case FR_OP_PMAP_CHK:
            if (*map & (1 << bit)) {
                DTRACE("DBG$ %d %s> fr id %d index %d, bit %d set for " \
                        "pid %d\n", currpid, __func__, fr_id, index, bit, pid);
                goto RESTORE_AND_RETURN_TRUE;
            } else {
                DTRACE("DBG$ %d %s> fr id %d index %d, bit %d not set for " \
                        "pid %d\n", currpid, __func__, fr_id, index, bit, pid);
                goto RESTORE_AND_RETURN_FALSE;
            }

        default:
            DTRACE("DBG$ %d %s> bad fr pmap oper %d\n",     \
                    currpid, __func__, oper);
            goto RESTORE_AND_RETURN_FALSE;
    }

RESTORE_AND_RETURN_TRUE:
    DTRACE_END;
    restore(ps);
    return TRUE;

RESTORE_AND_RETURN_FALSE:
    DTRACE_END;
    restore(ps);
    return FALSE;
}


/*******************************************************************************
 * Name:    evict_frm
 *
 * Desc:    Returns a new frame to use. If a free frame is available, returns
 *          that. Else, selects and evicts a frame based on the page 
 *          replacement policy and returns that frame. 
 *
 * Params:
 *  fr_type - type of the frame being requested (data/pdir/ptbl)
 *
 * Returns: 
 *  pointer to the new frame on success
 *  NULL on error
 ******************************************************************************/
frm_map_t *
evict_frm(fr_type)
{
    int new_fr_id = 0;
    int pr_policy = 0;
    frm_map_t *new_frame = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (FALSE == FR_IS_VALID_TYPE(fr_type)) {
        DTRACE("DBG$ %d %s> bad fr_type %d\n", currpid, __func__, fr_type);
        goto RESTORE_AND_RETURN_NULL;
    }

    /* If we are here, no free frames are available. Evict one of the frames
     * based on the page replacement algorithm that's being used.
     */
    pr_policy = grpolicy();
    switch (pr_policy) {
        case SC:
            new_fr_id = sc_evict_frm();
            new_frame = FR_GET_FPTR(new_fr_id);
            if (new_frame) {
                DTRACE("DBG$ %d %s> new_frame %d by pr_policy sc\n",    \
                        currpid, __func__, new_frame->fr_id);
                goto RESTORE_AND_RETURN_FRAME;
            } else {
                DTRACE("DBG$ %d %s> sc returned null frame\n",          \
                        currpid, __func__);
                goto RESTORE_AND_RETURN_NULL;
            }
            break;

        case NRU:
            new_fr_id = nru_evict_frm();
            new_frame = FR_GET_FPTR(new_fr_id);
            if (new_frame) {
                DTRACE("DBG$ %d %s> new_frame %d by pr_policy nru\n",   \
                        currpid, __func__, new_frame->fr_id);
                goto RESTORE_AND_RETURN_FRAME;
            } else {
                DTRACE("DBG$ %d %s> nru returned null frame\n",         \
                        currpid, __func__);
                goto RESTORE_AND_RETURN_NULL;
            }
            break;

        default:
            DTRACE("DBG$ %d %s> bad pr_policy %d\n",                    \
                   currpid, __func__, pr_policy);
            goto RESTORE_AND_RETURN_NULL;
    }

RESTORE_AND_RETURN_NULL:
    restore(ps);
    DTRACE_END;
    return NULL;

RESTORE_AND_RETURN_FRAME:
    init_frm(new_frame->fr_id);
    new_frame->fr_status = FR_INUSE;
    DTRACE_END;
    restore(ps);
    return new_frame;
}


/*******************************************************************************
 * Name:    get_frm 
 *
 * Desc:    Fetches a free frame, if available. If no free frames are available,
 *          replaces one of the frames in accordance with the page replacement
 *          policy in use.
 *
 * Params:
 *  fr_type - type of the frame being requested (data/pdir/ptbl)
 *
 * Returns: Pointer to the newly allocated frame. NULL if no frame is 
 *          available. 
 ******************************************************************************/
frm_map_t *
get_frm(int fr_type)
{
    int fr_id = 0;
    int rpolicy = 0;
    int upper_limit = 0;
    frm_map_t *new_frame = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (FALSE == FR_IS_VALID_TYPE(fr_type)) {
        DTRACE("DBG$ %d %s> bad fr_type %d\n", currpid, __func__, fr_type);
        goto RESTORE_AND_RETURN_NULL;
    }

    /* Go through all the free frames first. If one is available, return it.
     * Else, go for page replacement. Also, pt and pd frames can be 
     * allocated only iwthin the range of 1024 thry 1535. 
     */
    upper_limit = (FR_PAGE == fr_type) ? FR_NFRAMES : FR_TBL_LIMIT;
    if (FR_GET_NFREE > 0) {
        /* A free frame is available. Search thru and return it. */
        for (fr_id = 0; (fr_id < upper_limit); ++fr_id) {
            if (FR_FREE != FR_GET_STATUS(fr_id)) {
                continue;
            }

            new_frame = FR_GET_FPTR(fr_id);
            new_frame->fr_id = fr_id;
            DTRACE("DBG$ %d %s> returning free frame %d\n", \
                    currpid, __func__, new_frame->fr_id);
            goto RESTORE_AND_RETURN_FRAME;
        }
    }

    /* If we are here, there are no free frames in the system. Relpace an
     * existing frame. But, this should never happen for page directories or
     * tables. In such cases, print an error message and return NULL.
     */
    if (FR_PAGE != fr_type) {
        DTRACE("DBG$ %d %s> ERROR! Ran out of free frames for page "    \
                "tables/directories!\n", currpid, __func__);
        DTRACE_END;
        restore(ps);
        return NULL;
    }

    DTRACE("DBG$ %d %s> no more free frames.. going for replacement\n", \
            currpid, __func__);
    new_frame = evict_frm(fr_type);
    if (NULL == new_frame) {
        DTRACE("DBG$ %d %s> evict_frm() returned NULL\n", currpid, __func__);
        goto RESTORE_AND_RETURN_NULL;
    }

RESTORE_AND_RETURN_FRAME:
    rpolicy = grpolicy();
    if (SC == rpolicy) {
        /* Insert the allocated frame in to SC list. */
        sc_insert_frm(new_frame->fr_id);
    } else if (NRU == rpolicy) {
    }

    /* Populate few default fields. */
    new_frame->fr_status = FR_INUSE;
    new_frame->fr_ltime = ctr1000;

#ifdef DBG_ON
    DTRACE("DBG$ %d %s> before allocaing a new frame..\n", currpid, __func__);
    print_frm_data();
#endif

    /* Update frame counters. */
    FR_DEC_NFREE;
    FR_INC_NUSED;
    switch(fr_type) {
        case FR_PAGE:
            FR_INC_NPAGES;
            break;

        case FR_PTBL:
            FR_INC_NTABLES;
            break;

        case FR_PDIR:
            FR_INC_NDIRS;
            break;

        default:
            break;
    }
#ifdef DBG_ON
    DTRACE("DBG$ %d %s> after allocaing a new frame..\n", currpid, __func__);
    print_frm_data();
#endif

    DTRACE("DBG$ %d %s> returning valid new_frm\n", currpid, __func__);
    DTRACE_END;
    restore(ps);
    return new_frame;

RESTORE_AND_RETURN_NULL:
    DTRACE("DBG$ %d %s> returning NULL\n", currpid, __func__);
    DTRACE_END;
    restore(ps);
    return NULL;
}


/*******************************************************************************
 * Name:    free_frm
 *
 * Desc:    Frees the given frame. Clears the 'pres' bit in all the assoc.
 *          pgt entries if the frame is present in a pgt. Also, writes the
 *          frame to the appropriate BS if it is dirty. Does the following:
 *          1. Since the frame is being freed, we need to uppdate the page
 *              tables where the frame is present. The page table entry's
 *              'pres' flag will be turned off. Also, while reading the 
 *              page tables. we can also determine whether the frame is 
 *              dirty or not. If dirty, record it so that we can write off 
 *              the frame to BS later on.
 *          2. If the frame is dirty and if the frame happens to be a data
 *              frame, write it off to the appropriate BS.
 *          3. Reset frame meta data and change the state of the frame to free.
 *
 * Params:
 *  frame   - frame id of the frame to be freed.
 *
 * Returns: SYSCALL
 *  OK      - on success
 *  SYSERR  - on error
 ******************************************************************************/
SYSCALL 
free_frm(int fr_id)
{
    int is_dirty = EMPTY;
    frm_map_t *victim = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (FALSE == FR_IS_ID_VALID(fr_id)) {
        DTRACE("DBG$ %d %s> bad fr id %d\n", currpid, __func__, fr_id);
        goto RESTORE_AND_RETURN_ERROR;

    }

    victim = FR_GET_FPTR(fr_id);

#ifdef DBG_ON
    print_frm_id(fr_id);
#endif

    /* Clear the 'pres' bit on all the pgts that have a mapping to this
     * frame for all the processes.
     */
    if (EMPTY == (is_dirty = pgt_update_all(fr_id))) {
        DTRACE("DBG$ %d %s> pgt_update_all() returned EMPTY\n", \
                currpid, __func__, fr_id);
        goto RESTORE_AND_RETURN_ERROR;
    }

    /* Wirte the data frame to the BS if it's dirty. */
    if ((TRUE == is_dirty) && (FR_PAGE == FR_GET_TYPE(fr_id))) {
        if (SYSERR == write_bs((char *) FR_ID_TO_PA(fr_id), 
                    FR_GET_BS(fr_id), FR_GET_BSOFFSET(fr_id))) {
            DTRACE("DBG$ %d %s> write_bs() failed for fr id %d, "   \
                    "bs %d, offset %d\n", currpid, __func__, fr_id, \
                    victim->fr_bs, victim->fr_bsoffset);
        } else {
            DTRACE("DBG$ %d %s> writing to fr id %d to bs %d, offset %d\n", \
                    currpid, __func__, fr_id, victim->fr_bs,                \
                    victim->fr_bsoffset);
        }
    }

    /* Initialize the fields of the frame and update the counters. */
#ifdef DBG_ON
    DTRACE("DBG$ %d %s> before freeing a frame\n", currpid, __func__);
    print_frm_data();
#endif 
    FR_INC_NFREE;
    FR_DEC_NUSED;
    switch (FR_GET_TYPE(fr_id)) {
        case FR_PAGE:
            FR_DEC_NPAGES;
            break;
        case FR_PTBL:
            FR_DEC_NTABLES;
            break;
        case FR_PDIR:
            FR_DEC_NDIRS;
            break;
        default:
            break;
    }
    init_frm(fr_id);
    victim->fr_status = FR_FREE;
    DTRACE("DBG$ %d %s> fr id %d changed to FREE\n", currpid, __func__, fr_id);

#ifdef DBG_ON
    DTRACE("DBG$ %d %s> after freeing a frame\n", currpid, __func__);
    print_frm_data();
#endif 

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
 * Name:    is_frm_present 
 *
 * Desc:    Whenever a page fault happens, we need to load the page from BS. 
 *          Before that, we need to check if the requested page is shared with
 *          other processes and if so, if the frame corresponding to the shared
 *          page is present in the memory. This routine walks thru all the 
 *          frames and checks if the shared page is present in the memory.
 *
 * Params:
 *  bs_id   - id of the BS the shared page belongs to
 *  offset  - offset of the shared page within the BS
 *
 * Returns: int
 *  frame_id- if the shared page is present in the memory
 *  EMPTY   - otherwise
 ******************************************************************************/
int
is_frm_present(int bs_id, int offset) 
{
    int frame_id = 0;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (FALSE == BS_IS_ID_VALID(bs_id)) {
        DTRACE("DBG$ %d %s> bad bs id %d\n", currpid, __func__, bs_id);
        goto RESTORE_AND_RETURN_EMPTY;
    }

    /* Offset can be no less than 0 and no more than 127 as that's all a 
     * BS can map to.
     */
    if (FALSE == FR_IS_VALID_BSOFFSET(offset)) {
        DTRACE("DBG$ %d %s> bad bs offset %d\n", currpid, __func__, offset);
        goto RESTORE_AND_RETURN_EMPTY;
    }

    /* Go over all the frames and see if any of the data frames have the same
     * BS and offset. If so, he's our guy.
     */
    for (frame_id = 0; frame_id < FR_NFRAMES; ++frame_id) {
        if ((FR_FREE == FR_GET_STATUS(frame_id)) || 
                (FR_PTBL == FR_GET_TYPE(frame_id)) || 
                (FR_PDIR == FR_GET_TYPE(frame_id))) {
            continue;
        }

        if ((bs_id == FR_GET_BS(frame_id)) &&
                (offset == FR_GET_BSOFFSET(frame_id))) {
            /* Got our guy. */
            DTRACE("DBG$ %d %s> returning frame id %d\n",   \
                    currpid, __func__, frame_id);
            DTRACE_END;
            restore(ps);
            return frame_id;
        }
    }
    DTRACE("DBG$ %d %s> couldn't find a frame with bs id %d and offset %d\n",   \
            currpid, __func__, bs_id, offset);

RESTORE_AND_RETURN_EMPTY:
    DTRACE("DBG$ %d %s> returning EMPTY\n", currpid, __func__);
    DTRACE_END;
    restore(ps);
    return EMPTY;
}


/*******************************************************************************
 * Name:    inc_frm_refcount
 *
 * Desc:    Increments the refcount of the given frame. Usually called when a
 *          new frame is brought in or a process maps to an existing frame.
 *
 * Params:
 *  fr_id - id of the frame whose refcount is to be incremented 
 *
 * Returns: Nothing. 
 ******************************************************************************/
void
inc_frm_refcount(int fr_id)
{
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (FALSE == FR_IS_ID_VALID(fr_id)) {
        DTRACE("DBG$ %d %s> bad fr id %d\n", currpid, __func__, fr_id);
        goto END;
    }

    FR_INC_REFCOUNT(fr_id);
    DTRACE("DBG$ %d %s> fr id %d refcount incremented to %d\n", \
            currpid, __func__, fr_id, FR_GET_RCOUNT(fr_id));

END:
    DTRACE_END;
    restore(ps);
    return;
}


/*******************************************************************************
 * Name:    dec_frm_refcount
 *
 * Desc:    Decrements the refcount of the given frame. Usually called when a
 *          frame is paged out or a process unmaps an existing frame. If the 
 *          refcount reaches 0, it frees the frame.
 *
 * Params:
 *  fr_id - id of the frame whose refcount is to be decremented 
 *
 * Returns: Nothing. 
 ******************************************************************************/
void
dec_frm_refcount(int fr_id)
{
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (FALSE == FR_IS_ID_VALID(fr_id)) {
        DTRACE("DBG$ %d %s> bad fr id %d\n", currpid, __func__, fr_id);
        goto END;
    }

    FR_DEC_REFCOUNT(fr_id);
    DTRACE("DBG$ %d %s> fr id %d refcount decremented to %d\n", \
            currpid, __func__, fr_id, FR_GET_RCOUNT(fr_id));

    if (0 == FR_GET_RCOUNT(fr_id)) {
        DTRACE("DBG$ %d %s> fr id %d refcount reached 0.. freeing the frame\n", \
            currpid, __func__, fr_id);
        free_frm(fr_id);
    }

END:
    DTRACE_END;
    restore(ps);
    return;
}


/*******************************************************************************
 * Name:    frm_record_details 
 *
 * Desc:    Records the pid and vpage details in the given frame id. 
 *
 * Params:
 *  fr_id   - id of the frame whose refcount is to be decremented 
 *  pid     - pid to be recorded in the pidmap of the given frame
 *  vpage   - vpage to be associated with the pid for the frame
 *
 * Returns: Nothing. 
 ******************************************************************************/
void
frm_record_details(int fr_id, int pid, int vpage)
{
    frm_map_t *frptr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (FALSE == FR_IS_ID_VALID(fr_id)) {
        DTRACE("DBG$ %d %s> bad fr id %d\n", currpid, __func__, fr_id);
        goto RESTORE_AND_RETURN;
    }

    if (isbadpid(pid)) {
        DTRACE("DBG$ %d %s> bad pid %d\n", currpid, __func__, pid);
        goto RESTORE_AND_RETURN;
    }

    if (FALSE == VM_IS_VPAGE_VALID(vpage)) {
        DTRACE("DBG$ %d %s> bad vpage %d\n", currpid, __func__, vpage);
        goto RESTORE_AND_RETURN;
    }

    frptr = FR_GET_FPTR(fr_id);
    frm_pidmap_oper(fr_id, pid, FR_OP_PMAP_SET);
    frptr->fr_vpage[pid] = vpage;

    DTRACE("DBG$ %d %s> fr id %d updated with pid %d and vpage %d\n",   \
            currpid, __func__, fr_id, pid, frptr->fr_vpage[pid]);

RESTORE_AND_RETURN:
    DTRACE_END;
    restore(ps);
    return;
}
