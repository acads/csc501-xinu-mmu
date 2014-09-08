/* adhanas */
/* policy.c -- PA3, demand paging, SC and NRU page replacement algorithms */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

int page_replace_policy = SC;

/* SC supporting data structures. */
int sc_iter = 0;
int sc_tab[FR_NFRAMES];

/* NRU supporting structures. */
int nru_c0_ctr = 0;
int nru_c1_ctr = 0;
int nru_c2_ctr = 0;
int nru_c3_ctr = 0;
int nru_c0_tab[FR_NFRAMES];
int nru_c1_tab[FR_NFRAMES];
int nru_c2_tab[FR_NFRAMES];
int nru_c3_tab[FR_NFRAMES];

/****************************************************************************** 
 * Name:    srpolicy
 *
 * Desc:    Set the page replacement algorithm (SC or NRU) to be used. 
 *
 * Params:  
 *  policy  - page replacement algorithm to use
 *
 * Returns: SYSCALL
 *  OK      - on success
 *  SYSERR  - on failure
 *****************************************************************************/ 
SYSCALL 
srpolicy(int policy)
{
    DTRACE_START;
    if (SC == policy) {
        page_replace_policy = SC;
        DTRACE("DBG$ %d %s> policy set to SC %d\n", currpid, __func__, policy);
        kprintf("Test SC page replacement policy\n");
        sc_init_tab(FALSE);
    } else if (NRU == policy) {
        page_replace_policy = NRU;
        DTRACE("DBG$ %d %s> policy set to NRU %d\n", currpid, __func__, policy);
        kprintf("Test NRU page replacement policy\n");
        nru_init_tab(FALSE);
    } else {
        DTRACE("DBG$ %d %s> bad policy %d\n", currpid, __func__, policy);
        return SYSERR;
    }

    DTRACE_END;
    return OK;
}


/****************************************************************************** 
 * Name:    grpolicy
 *
 * Desc:    Returns the current page replacement algorithm (SC or NRU) in use. 
 *
 * Params:  None.
 *
 * Returns: Current pr policy in use (SC or NRU).
 *****************************************************************************/ 
int
grpolicy(void)
{
    DTRACE_START;
    DTRACE_END;
    return page_replace_policy;
}

/****************************************************************************** 
 * Name:    nru_init_tab 
 *
 * Desc:    Initializes the NRU tables. Usually called during init and when 
 *          the replacement policy is set to NRU. 
 *
 * Params:  
 *  sysinit - flag to indicate whether called from sysinit or by calling 
 *            srpolicy()
 *
 * Returns: Nothing.
 *****************************************************************************/ 
void
nru_init_tab(int sysinit)
{
    int fr_id = 0;

    DTRACE_START;

    /* Reset the NRU tables. */ 
    nru_c0_ctr = nru_c1_ctr = nru_c2_ctr = nru_c3_ctr = 0;
    for (fr_id = 0; fr_id < FR_NFRAMES; ++fr_id) {
        nru_c0_tab[fr_id] = EMPTY;
        nru_c1_tab[fr_id] = EMPTY;
        nru_c2_tab[fr_id] = EMPTY;
        nru_c3_tab[fr_id] = EMPTY;
    }
    DTRACE("DBG$ %d %s> nru tab initialized..\n", currpid, __func__);

    /* Build the table if NRU is selected in runtime. */
    if (FALSE == sysinit) {
        nru_build_tab();
    }

    DTRACE_END;
    return;
}


/****************************************************************************** 
 * Name:    nru_build_tab
 *
 * Desc:    Scans thru all data frames and classfies them into the following
 *          4 NRU classes based on their acc & dirty bit info.
 *
 *                CLASS |   ACC     |   DIRTY
 *                ------+-----------+--------
 *                  0   |   FALSE   |   FALSE
 *                  1   |   FALSE   |   TRUE
 *                  2   |   TRUE    |   FALSE
 *                  3   |   TRUE    |   TRUE
 *                ----------------------------
 *
 * Params:  None. 
 *
 * Returns: Nothing.
 *****************************************************************************/
void
nru_build_tab(void)
{
    int fr_id = EMPTY;
    int acc_bit = FALSE;
    int dirty_bit = FALSE;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    for (fr_id = 0; fr_id < FR_NFRAMES; ++fr_id) {
        if (FR_FREE == FR_GET_STATUS(fr_id)) {
            continue;
        }

        if (FR_PAGE != FR_GET_TYPE(fr_id)) {
            continue;
        }

        /* Fetch the acc & dirty bit values. */
        acc_bit = nru_is_bit_set(fr_id, PR_ACC_BIT);
        dirty_bit = nru_is_bit_set(fr_id, PR_DIRTY_BIT);

        /* Classify and insert the frame in appropriate table. */
        nru_insert_frm(fr_id, acc_bit, dirty_bit);
    }
    DTRACE("DBG$ %d %s> nru tab building.. done\n", currpid, __func__);

    DTRACE_END;
    restore(ps);
    return;
}


/****************************************************************************** 
 * Name:    nru_insert_frm 
 *
 * Desc:    Based on the given acc and dirty bit values, inserts the given
 *          frame into any one of the 4 NRU classes. 
 *
 * Params:  
 *  fr_id       - id of the frame which is to be inserted
 *  acc_bit     - acc bit value of the given frame
 *  dirty_bit   - dirty bit value of the given frame
 *
 * Returns: Nothing.
 *****************************************************************************/ 
void
nru_insert_frm(int fr_id, int acc_bit, int dirty_bit)
{
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (FALSE == FR_IS_ID_VALID(fr_id)) {
        DTRACE("DBG$ %d %s> bad fr id %d\n", currpid, __func__, fr_id);
        goto RESTORE_AND_RETURN;
    }

    if ((FALSE == acc_bit) && (FALSE == dirty_bit)) {
        nru_c0_tab[fr_id] = fr_id;
        PR_NRU_INC_C0_CTR;
        DTRACE("DBG$ %d %s> fr id %d put in nru c0, ctr %d\n",  \
                currpid, __func__, fr_id, PR_NRU_GET_C0_CTR);
    } else if ((FALSE == acc_bit) && (TRUE == dirty_bit)) {
        nru_c1_tab[fr_id] = fr_id;
        PR_NRU_INC_C1_CTR;
        DTRACE("DBG$ %d %s> fr id %d put in nru c1, ctr %d\n",  \
                currpid, __func__, fr_id, PR_NRU_GET_C1_CTR);
    } else if ((TRUE == acc_bit) && (FALSE == dirty_bit)) {
        nru_c2_tab[fr_id] = fr_id;
        PR_NRU_INC_C2_CTR;
        DTRACE("DBG$ %d %s> fr id %d put in nru c2, ctr %d\n",  \
                currpid, __func__, fr_id, PR_NRU_GET_C2_CTR);
    } else if ((TRUE == acc_bit) && (TRUE == dirty_bit)) {
        nru_c3_tab[fr_id] = fr_id;
        PR_NRU_INC_C3_CTR;
        DTRACE("DBG$ %d %s> fr id %d put in nru c3, ctr %d\n",  \
                currpid, __func__, fr_id, PR_NRU_GET_C3_CTR);
    } else {
        DTRACE("DBG$ %d %s> bad acc/dirty bit value.. cannot be classified\n", 
                currpid, __func__);
        goto RESTORE_AND_RETURN;
    }

RESTORE_AND_RETURN:
    DTRACE_END;
    restore(ps);
    return;
}


/****************************************************************************** 
 * Name:    nru_is_bit_set 
 *
 * Desc:    Scans the pidmap of the given frame and for each pid which uses 
 *          this frame, gets the given bit_type's value of this frame from the 
 *          pid's page tables. If the acc bit is set for at least one pid, we can
 *          safely retun without going thru other pids, as acc bit value of 1
 *          takes precedence over a value of 0.
 *
 * Params:  
 *  fr_id   - id of the frame whose bit value is required
 *  bit_type- type of bit to be checked for (acc/dirty)
 *
 * Returns: int
 *  TRUE    - if the given bit of the frame is set for at least one proc
 *  FALSY   - if the given bit of the frame is not set for any proc
 *  EMPTY   - oops! something went wrong
 *****************************************************************************/ 
int
nru_is_bit_set(int fr_id, int bit_type)
{
    int rc = FALSE;
    int pid = EMPTY;
    int vpage = EMPTY;
    int oper = EMPTY;
    frm_map_t *frptr = NULL;
    STATWORD ps; 

    DTRACE_START;

    if (FR_PAGE != FR_GET_TYPE(fr_id)) {
        DTRACE("DBG$ %d %s> sc_iter %d frm in sc_tab %d is not data\n",  \
                currpid, __func__, sc_iter, fr_id);
        rc = EMPTY;
        goto RESTORE_AND_RETURN;
    }

    /* Set the operation based on the given bit type. */
    switch (bit_type) {
        case PR_ACC_BIT:
            oper = PT_OP_ACC_BIT_CHK;
            break;

        case PR_DIRTY_BIT:
            oper = PT_OP_DIRTY_BIT_CHK;
            break;

        default:
            DTRACE("DBG$ %d %s> bad bit type %d\n",  \
                    currpid, __func__, bit_type);
            rc = EMPTY;
            goto RESTORE_AND_RETURN;
    }

    /* Go over all the procs and if the proc is set in the frame's pdimap,
     * fetch the bit type's value from the pid's page table.
     */
    for (pid = 1; pid < NPROC; ++pid) {
        if (PRFREE == P_GET_PSTATE(pid)) {
            continue;
        }

        /* If the bit is set for one proc, at least, we can safely return. */
        if (TRUE == rc) {
            DTRACE("DBG$ %d %s> fr id %d bit type %d is set\n", \
                    currpid, __func__, fr_id, bit_type);
            break;
        }


        /* Go over all the procs that uses the given frame and see if the given
         * bit is set or not. 
         */
        if (TRUE == frm_pidmap_oper(fr_id, pid, FR_OP_PMAP_CHK)) {
            DTRACE("DBG$ %d %s> fr id %d is used by pid %d\n", 
                    currpid, __func__, fr_id, pid);
            frptr = FR_GET_FPTR(fr_id);
            vpage = frptr->fr_vpage[pid];
            rc = pgt_oper(fr_id, pid, vpage, oper);
        }
    }

RESTORE_AND_RETURN:
    DTRACE_END;
    restore(ps);
    return rc; 
}


/****************************************************************************** 
 * Name:    nru_clr_acc_bit
 *
 * Desc:    In NRU, we need to clear the reference bits (acc bits) of the pages
 *          in the page tables in every 2 seconds. This function will be called
 *          every 2 seconds, which will go thru all the active procs page
 *          tables and clears the acc bit of data frames.
 *
 * Params:  None.
 *
 * Returns: Nothing.
 *****************************************************************************/ 
void
nru_clr_acc_bit(void)
{
    int pid = 0;
    int fr_id = 0;
    int vpage = EMPTY;
    frm_map_t *frptr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    DTRACE("DBG$ %d %s> timer %u\n", currpid, __func__, ctr1000);
    if (NRU != grpolicy()) {
        DTRACE("DBG$ %d %s> NRU is not the PR policy.. %d\n",
                currpid, __func__, grpolicy());
        goto RESTORE_AND_RETURN;
    }

    for (fr_id = 0; fr_id < FR_NFRAMES; ++fr_id) {
        if (FR_FREE == FR_GET_STATUS(fr_id)) {
            /* Don't bother about frames that are free already. */
            continue;
        }

        if (FR_PAGE != FR_GET_TYPE(fr_id)) {
            /* Don't bother about non-data frames. */
            continue;
        }

        for (pid = 0; pid < NPROC; ++pid) {
            if (PRFREE == P_GET_PSTATE(pid)) {
                continue;
            }

            /* Go over all the procs that uses the given frame and see if the given
             * bit is set or not. 
             */
            if (TRUE == frm_pidmap_oper(fr_id, pid, FR_OP_PMAP_CHK)) {
                DTRACE("DBG$ %d %s> fr id %d is used by pid %d\n", 
                        currpid, __func__, fr_id, pid);
                frptr = FR_GET_FPTR(fr_id);
                vpage = frptr->fr_vpage[pid];
                pgt_oper(fr_id, pid, vpage, PT_OP_ACC_BIT_CLR);
            }
        }
    }

RESTORE_AND_RETURN:
    /* Reset the timer before returning. */
    pr_nru_ticks = PR_NRU_TICKS;
    DTRACE_END;
    restore(ps);
    return;
}


/******************************************************************************* 
 * Name:    nru_evict_frm
 *
 * Desc:    Evicts a suitable frame from the list of available frames usin MRU
 *          algorithm. This routine searches the NRU classes in increasing
 *          order. i.e., it will search a higher class if and only if, no
 *          frames are available in the lower classes.
 *
 *          If more than a frame is present in the class, it selects the
 *          oldest frame. i.e., a frame with oldest load time timestamp.
 *
 *
 * Params:  None.
 *
 * Returns: int
 *          fr_id of the selected frame if everything goes thru fine
 *          EMPTY, otherwise
 ******************************************************************************/
int
nru_evict_frm(void)
{
    int fr_id = EMPTY;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (NRU != grpolicy()) {
        DTRACE("DBG$ %d %s> NRU is not the PR policy.. %d\n",
                currpid, __func__, grpolicy());
        goto RESTORE_AND_RETURN_EMPTY;
    }

    /* Rebuild NRU class tables. */
    nru_build_tab();

    /* Always select the lowest class for replacement. If no frames are 
     * available in the slected class, move over to the next.
     */
    if (0 != PR_NRU_GET_C0_CTR) {
        fr_id = nru_get_oldest_frm(nru_c0_tab);
        nru_c0_tab[fr_id] = EMPTY;
        PR_NRU_DEC_C0_CTR;
        DTRACE("DBG$ %d %s> fr id %d evicted from c0, ctr %d\n",    \
                currpid, __func__, fr_id, PR_NRU_GET_C0_CTR);
    } else if (0 != PR_NRU_GET_C1_CTR) {
        fr_id = nru_get_oldest_frm(nru_c1_tab);
        nru_c1_tab[fr_id] = EMPTY;
        PR_NRU_DEC_C1_CTR;
        DTRACE("DBG$ %d %s> fr id %d evicted from c1, ctr %d\n",    \
                currpid, __func__, fr_id, PR_NRU_GET_C1_CTR);
    } else if (0 != PR_NRU_GET_C2_CTR) {
        fr_id = nru_get_oldest_frm(nru_c2_tab);
        nru_c2_tab[fr_id] = EMPTY;
        PR_NRU_DEC_C2_CTR;
        DTRACE("DBG$ %d %s> fr id %d evicted from c2, ctr %d\n",    \
                currpid, __func__, fr_id, PR_NRU_GET_C2_CTR);
    } else if (0 != PR_NRU_GET_C3_CTR) {
        fr_id = nru_get_oldest_frm(nru_c3_tab);
        nru_c3_tab[fr_id] = EMPTY;
        PR_NRU_DEC_C3_CTR;
        DTRACE("DBG$ %d %s> fr id %d evicted from c3, ctr %d\n",    \
                currpid, __func__, fr_id, PR_NRU_GET_C3_CTR);
    } else {
        DTRACE("DBG$ %d %s> no frames in any of the classes.. what to do?\n", \
                currpid, __func__);
    }
    
    free_frm(fr_id);
    kprintf("Frame %d is replaced\n", (fr_id + FR_FRAME0));
    DTRACE_END;
    restore(ps);
    return fr_id;

RESTORE_AND_RETURN_EMPTY:
    DTRACE_END;
    restore(ps);
    return EMPTY;
}


/****************************************************************************** 
 * Name:    nru_get_oldest_frm 
 *
 * Desc:    Fetches the oldest frame (frame with the least load time) from
 *          the list of frames in the given class.
 *
 * Params:  
 *  nru_tab - pointer to the NRU class tab
 *
 * Returns: int
 *  fr_id   - id of the selected frame, if all goes good
 *  EMPTY   - something wrong (no frames in the class and so on..)
 *****************************************************************************/ 
int
nru_get_oldest_frm(int *nru_tab)
{
    int fr_id = EMPTY;
    int max_id = EMPTY;
    uint32_t max_ltime = PR_NRU_MAX_LTIME;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (NRU != grpolicy()) {
        DTRACE("DBG$ %d %s> NRU is not the PR policy.. %d\n",   \
                currpid, __func__, grpolicy());
        goto RESTORE_AND_RETURN_EMPTY;
    }

    if (NULL == nru_tab) {
        DTRACE("DBG$ %d %s> null nru tab\n", currpid, __func__, grpolicy());
        goto RESTORE_AND_RETURN_EMPTY;
    }

    for (fr_id = 0; fr_id < FR_NFRAMES; ++fr_id) {
        if (EMPTY == nru_tab[fr_id]) {
            continue;
        }

        if (FR_GET_LTIME(fr_id) < max_ltime) {
            max_ltime = FR_GET_LTIME(fr_id);
            max_id = fr_id;
        }
    }
    DTRACE("DBG$ %d %s> fr id %d is the oldest with ltime %u\n",    
            currpid, __func__, max_id, FR_GET_LTIME(max_id));

    DTRACE_END;
    restore(ps);
    return max_id;

RESTORE_AND_RETURN_EMPTY:
    DTRACE_END;
    restore(ps);
    return EMPTY;
}


/****************************************************************************** 
 * Name:    sc_init_tab 
 *
 * Desc:    Initializes the SC tab and it's selection pointers. Usually called
 *          during init and when the replacement policy is set to SC. 
 *
 * Params:  
 *  sysinit - flag to indicate whether called from sysinit or by calling 
 *            srpolicy()
 *
 * Returns: Nothing.
 *****************************************************************************/ 
void
sc_init_tab(int sysinit)
{
    int i = 0;

    DTRACE_START;

    /* Reset the SC table if and only if SC is not running. */
    if ((TRUE == sysinit) || (SC != grpolicy())) {
        sc_iter = 0;
        for (i = 0; i < FR_NFRAMES; ++i) {
            sc_tab[i] = EMPTY;
        }
        sc_iter = 0;
        DTRACE("DBG$ %d %s> sc tab initialized..\n", currpid, __func__);
    } else {
        DTRACE("DBG$ %d %s> sc is running already..\n", currpid, __func__);
        sc_build_tab();
    }

    DTRACE_END;
    return;
}


/****************************************************************************** 
 * Name:    sc_build_tab 
 *
 * Desc:    Based on the current frame table, it builds the SC table. This will
 *          called when the policy is set explicitly.
 *
 * Params:  None. 
 *
 * Returns: Nothing.
 *****************************************************************************/ 
void
sc_build_tab(void)
{
    int fr_id = 0;
    STATWORD ps;

    disable(ps);
    sc_iter = 0;

    for (fr_id = 0; fr_id < FR_NFRAMES; ++fr_id) {
        sc_tab[fr_id] = EMPTY;
    }

    for (fr_id = 0; fr_id < FR_NFRAMES; ++fr_id) {
        if (FR_INUSE == FR_GET_STATUS(fr_id)) {
            sc_tab[sc_iter] = fr_id;
            sc_inc_iter();
        }
    }
    restore(ps);
    return;
}


/****************************************************************************** 
 * Name:    sc_insert_frm
 *
 * Desc:    Inserts the given frame into the appropriate position in the the
 *          SC tab. 
 *
 * Params:
 *  fr_id   - id of the frame that is being inserted.
 *
 * Returns: Nothing.
 *****************************************************************************/ 
void
sc_insert_frm(int fr_id)
{
    STATWORD ps;

    disable(ps);
    DTRACE_START;
    DTRACE("DBG$ %d %s> sc iter is at %d in the start\n",  \
            currpid, __func__, sc_iter);

    if (SC != grpolicy()) {
        DTRACE("DBG$ %d %s> SC is not used as rpolicy.. %d\n",  \
                currpid, __func__, grpolicy());
        goto RESTORE_AND_RETURN;
    }


    if (EMPTY != sc_tab[sc_iter]) {
        DTRACE("DBG$ %d %s> sciter %d, sctab[sciter] = %d, bad value\n",  \
                currpid, __func__, sc_iter, sc_tab[sc_iter]);
    } else {
        sc_tab[sc_iter] = fr_id;
        sc_inc_iter();
    }

RESTORE_AND_RETURN:
    DTRACE("DBG$ %d %s> sc iter is at %d in the end\n",  \
            currpid, __func__, sc_iter);
    DTRACE_END;
    restore(ps);
    return;
}


/******************************************************************************* 
 * Name:    sc_evict_frm
 *
 * Desc:    Evicts a suitable frame from the list of available frames using
 *          second chance page replacement (SC) algorithm. This routine is
 *          responsible for the following:
 *          1. It goes over the list of frames and checks the accessed (acc) 
 *              bit of each data frame in all the pgts, as many processes could
 *              share the frame. 
 *          2. If any one of them is set, clear the acc bit in all pgts that
 *              happens to share the frame. Move to the next available frame
 *              in the list and goto step 1.
 *          3. If none of the pgts have the acc bit set, this frame could be
 *              freed. 
 *
 *          Going ove the list of frames (which are present in sc_tab) is
 *          done using an array iterator, sc_iter. 
 *
 * Params:  None.
 *
 * Returns: int
 *          fr_id of the selected frame if everything goes thru fine
 *          EMPTY, otherwise
 ******************************************************************************/
int
sc_evict_frm(void)
{
    int fr_id = EMPTY;
    int is_acc_set = FALSE;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    DTRACE("DBG$ %d %s> sc_iter is %d at start\n", currpid, __func__, sc_iter);

    fr_id = sc_tab[sc_iter];
    if (EMPTY == fr_id) {
        DTRACE("DBG$ %d %s> there are empty frames avaialble, but still "   \
                "page eviction is being called..\n", currpid, __func__);
        DTRACE("DBG$ %d %s> THIS SHOULD NEVER EVER HAPPEN!\n",  \
                currpid, __func__);
        goto RESTORE_AND_RETURN_EMPTY;
    }

    while (1) {
        fr_id = sc_tab[sc_iter];
        if (EMPTY == fr_id) {
            DTRACE("DBG$ %d %s> sc_iter %d frm in sc_tab EMPTY\n",  \
                    currpid, __func__, sc_iter);
            sc_inc_iter();
            continue;            
        }

        if (FR_PAGE != FR_GET_TYPE(fr_id)) {
            DTRACE("DBG$ %d %s> sc_iter %d frm in sc_tab %d is not data\n",  \
                    currpid, __func__, sc_iter, fr_id);
            sc_inc_iter();
            continue;
        }

        is_acc_set = sc_is_acc_bit_set(fr_id);
        if (FALSE == is_acc_set) {
            DTRACE("DBG$ %d %s> sc_iter %d frm in sc_tab %d acc bit not set\n",\
                    currpid, __func__, sc_iter, fr_id);
            /* This frame can be evicted. */
            sc_tab[sc_iter] = EMPTY;
            free_frm(fr_id);
            break;
        } else {
            DTRACE("DBG$ %d %s> sc_iter %d frm in sc_tab %d acc bit set\n",\
                    currpid, __func__, sc_iter, fr_id);
            sc_clr_acc_bit(fr_id);
            sc_inc_iter();
            continue;
        }
    }

    DTRACE("DBG$ %d %s> sc_iter %d, fr id %d is being returned\n",
            currpid, __func__, sc_iter, fr_id);
    kprintf("Frame %d is replaced\n", (fr_id + FR_FRAME0));
    DTRACE_END;
    restore(ps);
    return fr_id;

RESTORE_AND_RETURN_EMPTY:
    DTRACE_END;
    restore(ps);
    return EMPTY;
}


/*******************************************************************************
 * Name:    sc_inc_iter
 *
 * Desc:    Increments the sc_iter. If it reaches the end (NR_FRMAES - 1), it
 *          woll be set to 0.
 *
 * Params:  None.
 *
 * Returns: Nothing.
 ******************************************************************************/
void
sc_inc_iter(void)
{
    DTRACE_START;
    sc_iter += 1;
    if ((FR_NFRAMES - 1) == sc_iter) {
        /* Time to bring it back to 0. */
        DTRACE("DBG$ %d %s> sciter set to 0..\n", currpid, __func__);
        sc_iter = 0;
    }
    DTRACE_END;
    return;
}


/*******************************************************************************
 * Name:    sc_is_acc_bit_set 
 *
 * Desc:    Goes over the pgts of the procs that share the given data frame.
 *          If the acc bit is set for any one of the procs, returns TRUE.
 *
 * Params: 
 *  fr_id   - id of the frame whose acc bit is to be checked
 *
 * Returns: int
 *  TRUE    - if acc bit set for any one of the procs
 *  FALSE   - if acc bit not set for any of the procs
 *  EMPTY   - if anything goes wrong (given frame is not a data frame)
 ******************************************************************************/
int
sc_is_acc_bit_set(int fr_id)
{
    int rc = FALSE;
    int pid = EMPTY;
    frm_map_t *frptr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (FR_PAGE != FR_GET_TYPE(fr_id)) {
        DTRACE("DBG$ %d %s> sc_iter %d frm in sc_tab %d is not data\n",  \
                currpid, __func__, sc_iter, fr_id);
        rc = EMPTY;
        goto RESTORE_AND_RETURN;
    }

    for (pid = 1; pid < NPROC; ++pid) {
        if (PRFREE == P_GET_PSTATE(pid)) {
            continue;
        }

        if (TRUE == rc) {
            DTRACE("DBG$ %d %s> fr id %d acc bit is set\n", currpid, __func__);
            break;
        }


        /* Go over all the procs that uses the given frame and see if the acc
         * bit is set or not. If it's set for at least one proc, we can safely
         * return whithout checking the remaining procs.
         */
        if (TRUE == frm_pidmap_oper(fr_id, pid, FR_OP_PMAP_CHK)) {
            DTRACE("DBG$ %d %s> fr id %d is used by pid %d\n", 
                    currpid, __func__, fr_id, pid);
            frptr = FR_GET_FPTR(fr_id);
            rc = pgt_oper(fr_id, pid, frptr->fr_vpage[pid], PT_OP_ACC_BIT_CHK);
        }
    }

RESTORE_AND_RETURN:
    DTRACE_END;
    restore(ps);
    return rc;
}


/*******************************************************************************
 * Name:    sc_clr_acc_bit 
 *
 * Desc:    Clears the acc bit for the given frame in all the pgts of the procs
 *          that happens to share this frame.
 *
 * Params:  
 *  fr_id   - id of the frame whose acc bit is to be cleared
 *
 * Returns: Nothing.
 ******************************************************************************/
void
sc_clr_acc_bit(int fr_id)
{
    int pid = EMPTY;
    int vpage = EMPTY;
    frm_map_t *frptr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (FR_PAGE != FR_GET_TYPE(fr_id)) {
        DTRACE("DBG$ %d %s> sc_iter %d frm in sc_tab %d is not data\n",  \
                currpid, __func__, sc_iter, fr_id);
        goto RESTORE_AND_RETURN;
    }

    for (pid = 1; pid < NPROC; ++pid) {
        if (PRFREE == P_GET_PSTATE(pid)) {
            continue;
        }

        /* Go over all the procs that uses the given frame and clear the acc
         * bit if it's not set.
         */
        if (TRUE == frm_pidmap_oper(fr_id, pid, FR_OP_PMAP_CHK)) {
            DTRACE("DBG$ %d %s> fr id %d is used by pid %d\n", 
                    currpid, __func__);
            frptr = FR_GET_FPTR(fr_id);
            vpage = frptr->fr_vpage[pid];
            pgt_oper(fr_id, pid, vpage, PT_OP_ACC_BIT_CLR);
        }
    }

RESTORE_AND_RETURN:
    DTRACE_END;
    restore(ps);
    return;
}

#if 0
void
nru_timer(void)
{
    STATWORD ps;

    disable(ps);
    if (grpolicy() != NRU) {
        kprintf("Not NRU.. %d\n", ctr1000);
        pr_nru_ticks = PR_NRU_TICKS;
        restore(ps);
        return;
    }
    kprintf("Timer works %u\n", ctr1000);
    pr_nru_ticks = PR_NRU_TICKS;
    restore(ps);
    return;
}
#endif
