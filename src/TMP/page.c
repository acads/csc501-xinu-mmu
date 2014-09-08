/* adhanas */
/* page.c -- PA3, implements page tables and directories */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

pt_t *g_pt[PT_NGPT];    /* global page tables */

/*******************************************************************************
 * Name:    init_pgt
 *
 * Desc:    Intializes a page table. 
 *
 * Params:
 *  ptptr   - pointer to the page table that is to be initialized.
 *
 * Returns: Nothing.
 ******************************************************************************/
void
init_pgt(pt_t *ptptr)
{
    int pt_entry_id = 0;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (NULL == ptptr) {
        DTRACE("DBG$ %d %s> null in: ptptr\n", currpid, __func__);
        goto RESTORE_AND_RETURN;
    }

    /* Each page table entry is of 4 bytes long. Each page is 4096 bytes long
     * and thus, each page can hold (4096/4 = 1024) page entries.
     * All fields will be set to zero. The caller should set the required
     * fields themselves.
     */
    for (pt_entry_id = 0; pt_entry_id < PT_PER_FRAME; ++pt_entry_id) {
        ptptr[pt_entry_id].pt_pres = 0;
        ptptr[pt_entry_id].pt_write = 0;
        ptptr[pt_entry_id].pt_user = 0;
        ptptr[pt_entry_id].pt_pwt = 0;
        ptptr[pt_entry_id].pt_pcd = 0;
        ptptr[pt_entry_id].pt_acc = 0;
        ptptr[pt_entry_id].pt_dirty = 0;
        ptptr[pt_entry_id].pt_mbz = 0;
        ptptr[pt_entry_id].pt_global = 0;
        ptptr[pt_entry_id].pt_avail = 0;
        ptptr[pt_entry_id].pt_base = 0;
    }
    DTRACE("DBG$ %d %s> pgt initialized\n", currpid, __func__);

RESTORE_AND_RETURN:
    DTRACE_END;
    restore(ps);
    return;
}


/*******************************************************************************
 * Name:    init_global_pgt
 *
 * Desc:    Intializes the global page tables. Usually called during system init 
 *          and when a new page table is created. Creates 4 global page tables
 *          that maps the actual physical memory of 16 MB. These tables are
 *          memory resident and will be shared by all the processes.
 *
 * Params:  None.
 *
 * Returns: Nothing.
 ******************************************************************************/
void
init_global_pgt(void)
{
    int gpt_id = 0;
    int pt_entry_id = 0;
    pt_t *ptptr = NULL;
    STATWORD ps;
    
    DTRACE_START;

    /* Allocate frames for the global page tables. Each entry in the page table
     * is 4 bytes long and each pframe is 4096 bytes long. Thus, we need 4 frames
     * to store 4 global page tables.
     */
    for (gpt_id = 0; gpt_id < PT_NGPT; ++gpt_id) {
        g_pt[gpt_id] = ptptr = new_pgt();
        if (NULL == ptptr) {
            DTRACE("DBG$ %d %s> new_pgt()) failed\n", currpid, __func__);
            goto RESTORE_AND_RETURN;
        }

        /* new_pgt() would have initialized all the fields of pgt to zero. We
         * neeed to set the required fields here.
         */
        for (pt_entry_id = 0; pt_entry_id < PT_PER_FRAME; ++pt_entry_id) {
            ptptr[pt_entry_id].pt_pres = 1;
            ptptr[pt_entry_id].pt_write = 1;
            ptptr[pt_entry_id].pt_base = ((gpt_id * PT_PER_FRAME) + 
                                                pt_entry_id); 

            if (0 == pt_entry_id) {
                DTRACE("DBG$ %d %s> global pt is at 0x%08x\n",      \
                        currpid, __func__, g_pt[gpt_id], g_pt[gpt_id]);
            }
        }
    }
    DTRACE("DBG$ %d %s> global pgt initialized\n", currpid, __func__);

RESTORE_AND_RETURN:
    DTRACE_END;
    restore(ps);
    return;
}


/*******************************************************************************
 * Name:    new_pgt 
 *
 * Desc:    Allocates and intializes a new page table.
 *
 * Params:  None.
 *
 * Returns: Pointer to the newly allocated pgt. NULL on error.
 ******************************************************************************/
pt_t *
new_pgt(void)
{
    pt_t *new_pt = NULL;
    frm_map_t *new_frame = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    new_frame = get_frm(FR_PTBL);
    if (NULL == new_frame) {
        DTRACE("DBG$ %d %s> get-frm() failed\n", currpid, __func__);
        kprintf("\n\n");
        kprintf("FATAL ERROR: Ran out of free frames for page tables!\n");
        kprintf("Process '%s' with PID '%d' will be terminated.\n", \
                P_GET_PNAME(currpid), currpid);
        kprintf("\n\n");
        sleep(9);
        DTRACE_END;
        restore(ps);
        kill(getpid());
        goto RESTORE_AND_RETURN_NULL;
    }
    new_frame->fr_type = FR_PTBL;

    /* The PT would start at the PA of the frame that we just got. Since, it's
     * coniguous, we can fill-in the values in a looped manner.
     */
    new_pt = (pt_t *) FR_ID_TO_PA(new_frame->fr_id);
    init_pgt(new_pt);

    DTRACE("DBG$ %d %s> returning new pgt at 0x%08x\n", \
            currpid, __func__, new_pt);
    DTRACE_END;
    restore(ps);
    return new_pt;

RESTORE_AND_RETURN_NULL:
    DTRACE("DBG$ %d %s> returning NULL\n", currpid, __func__);
    DTRACE_END;
    restore(ps);
    return NULL;
}


/*******************************************************************************
 * Name:    new_pgd
 *
 * Desc:    Allocates and intializes a new page directory. Usually called during
 *          process creation.
 *
 * Params:  None.
 *
 * Returns: Pointer to the newly allocated pgd. NULL on error.
 ******************************************************************************/
pd_t *
new_pgd(void)
{
    int pd_id = 0;
    pd_t *new_pd = NULL;
    frm_map_t *new_frame = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    new_frame = get_frm(FR_PDIR);
    if (NULL == new_frame) {
        DTRACE("DBG$ %d %s> get_frm() failed\n", currpid, __func__);
        goto RESTORE_AND_RETURN_NULL;
    }
    new_frame->fr_type = FR_PDIR;

    /* The PD would start at the PA of the frame that we just got. */
    new_pd = (pd_t *) FR_ID_TO_PA(new_frame->fr_id);

    /* Same as page tables, each pd entry is 4 bytes long. Thus, a frame which 
     * is 4 KB, can hold (NBPG / 4 = 1024) entries. Intially all the fields 
     * are set to zero. Also, every process will share the global page tables.
     */
    bzero(new_pd, sizeof(pd_t));
    for (pd_id = 0; pd_id < PT_NGPT; ++pd_id) {
        new_pd[pd_id].pd_pres = 1;
        new_pd[pd_id].pd_write = 1;
        new_pd[pd_id].pd_avail = 1;
        /* The base should contain vpage of the frame containing the global
         * page tables.
         */
        new_pd[pd_id].pd_base = VADDR_TO_VPAGE((unsigned) g_pt[pd_id]);
    }
    DTRACE("DBG$ %d %s> pgd at 0x%08x is being returned\n",  \
            currpid, __func__, new_pd);

    DTRACE_END;
    restore(ps);
    return new_pd;

RESTORE_AND_RETURN_NULL:
    DTRACE_END;
    restore(ps);
    return NULL;
}


/*******************************************************************************
 * Name:    remove_pgd
 *
 * Desc:    Removes the pgd of a proc and frees the frame. 
 *
 * Params:  
 *  pid     - pid whose pgd has to be removed
 *
 * Returns: Nothing. 
 ******************************************************************************/
void
remove_pgd(int pid)
{
    int pgd_fr_id = EMPTY;
    STATWORD ps;

    disable(ps);
    DTRACE_START;
    if (isbadpid(pid) || (PRFREE == P_GET_PSTATE(pid))) {
        DTRACE("DBG$ %d %s> bad pid %d or bad state %d\n",  \
                currpid, __func__, pid, P_GET_PSTATE(pid));
        goto RESTORE_AND_RETURN;
    }

    /* Each process has a pdir whose base addr is stored in pgdir field of 
     * the PCB. This is hosted on a frame and we need to free that frame as the
     * process is being killed.
     */
    pgd_fr_id = FR_PA_TO_ID(P_GET_PDIR(pid));
    free_frm(pgd_fr_id);

RESTORE_AND_RETURN:
    DTRACE_END;
    restore(ps);
    return;
}


/*******************************************************************************
 * Name:    pgt_update_all
 *
 * Desc:    Called when a frame is freed. This routine updates the page tables
 *          of all the active procs that uses the given frame. It usually clears
 *          the page table entry for the given frame and records the dirty bit
 *          value.
 *
 * Params:  
 *  fr_id   - frame id of the frame being paged out/removed
 *
 * Returns: int
 *  TRUE    - if the dirty bit is set for at least one proc
 *  FALSE   - if the dirty bit is not set for any of the procs
 *  EMPTY   - oops! something went wrong..
 ******************************************************************************/
int
pgt_update_all(int fr_id)
{
    int rc = FALSE;
    int pid = 0;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (FALSE == FR_IS_ID_VALID(fr_id)) {
        DTRACE("DBG$ %d %s> bad fr id %d\n", currpid, __func__, fr_id);
        rc = EMPTY;
        goto RESTORE_AND_RETURN;
    }

    /* If the given frame is free, the required pgt's would have been updated
     * earlier. Just return.
     */
    if (FR_FREE == FR_GET_STATUS(fr_id)) {
        DTRACE("DBG$ %d %s> fr id %d is free\n",    \
                currpid, __func__, fr_id, FR_GET_STATUS(fr_id));
        rc = FALSE;
        goto RESTORE_AND_RETURN;
    }

    /* This routine is only to update the page tables when a frame is removed
     * or a proc unmaps the frame. So, we only have to bother about data pages.
     */
    if (FR_PAGE != FR_GET_TYPE(fr_id)) {
        DTRACE("DBG$ %d %s> fr id %d isn't a data frame.. type %d\n",   \
                currpid, __func__, fr_id, FR_GET_TYPE(fr_id));
        rc = FALSE;
        goto RESTORE_AND_RETURN;
    }

    for (pid = 0; pid < NPROC; ++pid) {
        if (PRFREE == P_GET_PSTATE(pid)) {
            /* Don't bother if a proc isn't present. */
            continue;
        }

        if (TRUE == frm_pidmap_oper(fr_id, pid, FR_OP_PMAP_CHK)) {
            DTRACE("DBG$ %d %s> fr id %d is used by pid %d\n",
                    currpid, __func__, fr_id, pid);
            rc |= pgt_update_for_pid(fr_id, pid);
        }
    }

RESTORE_AND_RETURN:
    DTRACE_END;
    restore(ps);
    return rc;
}


/*******************************************************************************
 * Name:    pgt_update_for_pid
 *
 * Desc:    Called when a frame is freed. This routine:
 *          1. Goes over all the pgts of the given process and clears the 
 *              'pres' bit of the page table entry as the frame is about to 
 *              be freed, eventually.
 *          2. As it goes over the page tables of the process, it records
 *              the dirty bit value of the page table. This will be 
 *              returned to the caller so that caller can write-off the frame
 *              to the BS. 
 *          3. If all the frames of the page table frame are freed, the page
 *              table frame itself can be freed. In this case, we need to 
 *              update the 'pres' bit of the corrsponding entry in the page 
 *              directory (outer page table) of the process.
 *          4. Processor caches the paging entries (TLB) maintained by software
 *              and uses them whenever possible. When a pgt entry's 'pres' bit
 *              is cleared, we need to flush the entry from the processor 
 *              cache so that the proceessor would use the updated software
 *              data. This is described in detail in section 4.8 of Intel IA32 
 *              software developer manual (vol 3). There are many ways to force
 *              the processor to use the software tables, than hardware cache.
 *              One such way is to reload teh CR0 register. So, if any of the
 *              'pres' bits are modified, we reload the CR0 register.
 *
 * Params:  
 *  fr_id   - frame id of the frame being paged out/removed
 *  pid     - pid of the proc whose page table is to be updated (optional)
 *              passing EMPTY would update all pid page tables
 *
 * Returns: int
 *  TRUE    - if the dirty bit is set for at least one proc
 *  FALSE   - if the dirty bit is not set for any of the procs
 *  EMPTY   - oops! something went wrong..
 ******************************************************************************/
int
pgt_update_for_pid(int fr_id, int pid)
{
    int rc = FALSE;
    frm_map_t *frptr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (FALSE == FR_IS_ID_VALID(fr_id)) {
        DTRACE("DBG$ %d %s> bad fr id %d\n", currpid, __func__, fr_id);
        rc = EMPTY;
        goto RESTORE_AND_RETURN;
    }

    /* If the given frame is free, the required pgt's would have been updated
     * earlier. Just return.
     */
    if (FR_FREE == FR_GET_STATUS(fr_id)) {
        DTRACE("DBG$ %d %s> fr id %d is free\n",    \
                currpid, __func__, fr_id, FR_GET_STATUS(fr_id));
        rc = FALSE;
        goto RESTORE_AND_RETURN;
    }

    /* This routine is only to update the page tables when a frame is removed
     * or a proc unmaps the frame. So, we only have to bother about data pages.
     */
    if (FR_PAGE != FR_GET_TYPE(fr_id)) {
        DTRACE("DBG$ %d %s> fr id %d isn't a data frame.. type %d\n",   \
                currpid, __func__, fr_id, FR_GET_TYPE(fr_id));
        rc = FALSE;
        goto RESTORE_AND_RETURN;
    }

    if (isbadpid(pid)) {
        DTRACE("DBG$ %d %s> bad pid %d\n", currpid, __func__, pid);
        rc = EMPTY;
        goto RESTORE_AND_RETURN;
    }

    if (PRFREE == P_GET_PSTATE(pid)) {
        DTRACE("DBG$ %d %s> pid %d is in free state\n", \
                currpid, __func__, pid);
        rc = EMPTY;
        goto RESTORE_AND_RETURN;
    }

    /* If the pid doesn't use the given frame, return FALSE. */
    if (FALSE == frm_pidmap_oper(fr_id, pid, FR_OP_PMAP_CHK)) {
        DTRACE("DBG$ %d %s> fr id %d is not used by pid %d\n",
                currpid, __func__, fr_id, pid);
        rc = FALSE;
        goto RESTORE_AND_RETURN;
    }

    /* If we are here, we are good to go. Update the page table by doing the
     * following:
     *  1. Record the dirty bit value.
     *  2. Clear the 'pres' bit for the page in the page table.
     *  3. Decrement the refcount of the page table.
     *  4. If the refcount of the frame containing the page table reaches 0,
     *      free the frame.
     *  5. If the frame containing the page table is freed, clear the 'pres'
     *      bit of the page directory entry of the process containing the just
     *      freed frame.
     */
    frptr = FR_GET_FPTR(fr_id);
    rc = pgt_oper(fr_id, pid, frptr->fr_vpage[pid], PT_OP_FREE_FRM);

RESTORE_AND_RETURN:
    /* Reload the CR0 register would force the processor to flush the tables
     * that the processor maintains in hardware cache and to use the updated
     * software tables.
     */
    enable_paging();
    DTRACE_END;
    restore(ps);
    return rc;
}


/******************************************************************************
 * Name:    pgt_oper
 *
 * Desc:    Given frame id, pid and vpage, this routine can perform the 
 *          following operations on the page table entry associated with the
 *          given frame ID of the given process:
 *              1. Get the value of the dirty bit.
 *              2. Get the value of the acc bit.
 *              3. Clear the acc bit.
 *              4. Related operations to be perfromed on the page table
 *                  when a mapped frame is freed.
 *
 *          The given vpage will be first converted into vaddr, from which the
 *          offsets for page directory, page table and the actual page are
 *          figured out. From the pid, the base address of the page directory
 *          is figured out. With these info, the actual page table entry can
 *          be directly accessed.
 *
 * Params:  
 *  fr_id   - id of the frame whose acc bit value is to be figured out
 *  pid     - pid of the proc whose page tables are to be searched for
 *  vpage   - associated vpage of this frame with the pid
 *  oper    - get dirty, get acc or clear acc
 *
 * Returns: int
 *  TRUE    - if the acc/dirty bit is set and the oper is get acc/dirty/clear
 *  FALSE   - if the acc/dirty bit is not set and the oper is get acc/dir
 *  EMPTY   - if something goes wrong (pid is free, fr id is free and so on)
 *****************************************************************************/
int
pgt_oper(int fr_id, int pid, int vpage, int oper)
{
    int rc = FALSE;
    int pd_id = 0;
    int pt_id = 0;
    int pg_id = 0;
    int pt_fr_id = 0;
    int pt_pres = 0;
    int pg_pres = 0;
    int pg_acc = 0;
    int pg_dirty = 0;
    uint32_t vaddr = 0;
    uint32_t base_page = 0;
    virt_addr_t *vaddrptr = NULL;
    pt_t *ptbl = NULL;
    pd_t *pdir = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;
    printf("pgtoper: OPER: %d\n", oper);

    if (FALSE == FR_IS_ID_VALID(fr_id)) {
        DTRACE("DBG$ %d %s> bad fr id %d\n", currpid, __func__, fr_id);
        rc = EMPTY;
        goto RESTORE_AND_RETURN;
    }

    if (PRFREE == P_GET_PSTATE(pid)) {
        DTRACE("DBG$ %d %s> pid %d is free\n", currpid, __func__, pid);
        rc = EMPTY;
        goto RESTORE_AND_RETURN;
    }

    if (FALSE == VM_IS_VPAGE_VALID(vpage)) {
        DTRACE("DBG$ %d %s> bad vpage %d\n", currpid, __func__, vpage);
        rc = EMPTY;
        goto RESTORE_AND_RETURN;
    }

    /* Convert the vpage to vaddr and fetch the required table offsets. */
    vaddr = VPAGE_TO_VADDR(vpage);
    vaddrptr = (virt_addr_t *) (&vaddr);
    pd_id = (uint32_t) vaddrptr->pd_offset;
    pt_id = (uint32_t) vaddrptr->pt_offset;
    pg_id = (uint32_t) vaddrptr->pg_offset;

    DTRACE("DBG$ %d %s> fr id %d, pid %d, vpage %d, oper %d\n", \
            currpid, __func__, fr_id, pid, vpage, oper);
    DTRACE("DBG$ %d %s> vaddr 0x%08x, pd_id %d, pt_id %d, pg_id %d\n",  \
            currpid, __func__, vaddr, pd_id, pt_id, pg_id);

    /* Get the page directory. */
    pdir = P_GET_PDIR(pid);
    if (NULL == pdir) {
        DTRACE("DBG$ %d %s> pdir is NULL for pid %d\n", currpid, __func__, pid);
        rc = EMPTY;
        goto RESTORE_AND_RETURN;
    }

    /* Get the page table, if available. */
    if (FALSE == PD_GET_PRES(pdir, pd_id)) {
        /* Page table is not present. Record it for now. Based on the oper,
         * we can error out, if required, later.
         */
        pt_pres = FALSE;
    } else {
        /* Page table is present, fetch the base address. */
        pt_pres = TRUE;
        ptbl = (pt_t *) VPAGE_TO_VADDR(PD_GET_BASE(pdir, pd_id));
        pt_fr_id = FR_PA_TO_ID(ptbl);

        /* Get the page, if available. */
        if (FALSE == PT_GET_PRES(ptbl, pt_id)) {
            pg_pres = FALSE;
        } else {
            pg_pres = TRUE;
            base_page = FR_ID_TO_VPAGE(fr_id);
            if (base_page == PT_GET_BASE(ptbl, pt_id)) {
                pg_pres = TRUE;
                pg_acc = PT_GET_ACC(ptbl, pt_id);
                pg_dirty = PT_GET_DIRTY(ptbl, pt_id);
            } else {
                pg_pres = FALSE;
                pg_acc = EMPTY;
            }
        }
    }

    DTRACE("DBG$ %d %s> pid %d, fr_id %d, vpage %d, vaddr 0x%08x, "     \
            "pd_id %d, pt_id %d, pg_id %d\n", currpid, __func__, pid,   \
            fr_id, vpage, vaddr, pd_id, pt_id, pg_id);
    switch (oper) {
        case PT_OP_ACC_BIT_CHK:
            if (pg_pres) {
                rc = pg_acc = PT_GET_ACC(ptbl, pt_id);
            } else {
                /* The page itslef is not present. */
                rc = pg_acc = EMPTY;
            }
            DTRACE("DBG$ %d %s> PT_OP_ACC_BIT_CHK: %d\n",   \
                    currpid, __func__, rc);
            break;

        case PT_OP_ACC_BIT_CLR:
            if (pg_pres) {
                ptbl[pt_id].pt_acc = 0;
                rc = TRUE;
            } else {
                rc = EMPTY;
            }
            DTRACE("DBG$ %d %s> PT_OP_ACC_BIT_CLR: %d\n",   \
                    currpid, __func__, rc);
            break;

        case PT_OP_DIRTY_BIT_CHK:
            if (pg_pres) {
                rc = pg_dirty = PT_GET_DIRTY(ptbl, pt_id);
            } else {
                rc = pg_dirty = EMPTY;
            }
            DTRACE("DBG$ %d %s> PT_OP_DIRTY_BIT_CHK: %d\n",   \
                    currpid, __func__, rc);
            break;

        case PT_OP_FREE_FRM:
            /* A frame is being freed. Following needs to be performed on pgt:
             *  1. Record the dirty bit value.
             *  2. Clear the 'pres' bit for the page in the page table.
             *  3. Decrement the refcount of the page table.
             *  4. If the refcount of the frame containing the page table 
             *      reaches 0, free the frame.
             *  5. If the frame containing the page table is freed, clear the 
             *      'pres' bit of the page directory entry of the process 
             *      containing the just freed frame.
             */
            if (pg_pres) {
                rc = pg_dirty;
                ptbl[pt_id].pt_pres = 0;
                dec_frm_refcount(pt_fr_id);
                DTRACE("DBG$ %d %s> pid %d, pd %d, pt %d, fr id %d, "   \
                        "dirty %d\n", currpid, __func__, pid, pd_id,    \
                        pt_id, fr_id, rc);
                DTRACE("DBG$ %d %s> clearing pres bit in pgt for "  \
                        "pid %d, pd %d, pt %d, fr id %d\n",        \
                        currpid, __func__, pid, pd_id, pt_id, fr_id);

                /* Clear the 'pres' bit of the this pgt entry in the proc's
                 * pdir, if all the pages of the pgt are removed from the
                 * memory.
                 */
                if (FR_FREE == FR_GET_STATUS(pt_fr_id)) {
                    DTRACE("DBG$ %d %s> clearing pres bit in pgd for "  \
                            "pid %d, pd %d, fr id  %d\n",               \
                            currpid, __func__, pid, pd_id, pt_fr_id);
                    pdir[pd_id].pd_pres = 0; 
                }
            } else {
                rc = EMPTY;
            }
            break;

        default:
            DTRACE("DBG$ %d %s> bad pt oper %d\n", currpid, __func__, oper);
            rc = EMPTY;
            goto RESTORE_AND_RETURN;
    }


RESTORE_AND_RETURN:
    DTRACE_END;
    restore(ps);
    return rc;
}


/*******************************************************************************
 * Name:    pf_handler 
 *
 * Desc:    High level page fault handling code. The low level page fault
 *          interrupt handler is written in assembly. It'll setup the required
 *          registers (faulted address in CR2) and the stack frame with the 
 *          error code. This routine is responsible for the following:
 *          1. Read the faulted address and lookup BS to find if this address is
 *              mapped. If so, get the BS id and the offset within the BS. If 
 *              not, this is an illegal access - kill the process and move on.
 *          2. Actual paging starts here. Lookup the proctab to find the pid's
 *              pdir base. From the faulted address, we can get the pdir offset.
 *              Using these two, check if a page table exist for the faulted
 *              address. If not create one.
 *          3. If the page table is presnet, get the ptbl offset from the 
 *              faulted vaddr. This points to the location of the page table
 *              entry. Now, check if the frame assoicated with this page is
 *              already present in the memory (shared pages). If so, update the
 *              page table's 'pres' bit to reflect this and increment the 
 *              frame's refcount. If not, allocate a new frame and update the
 *              page table entry to reflect that the frame is present.
 *          4. Processor caches the paging entries (TLB) maintained by software
 *              and uses them whenever possible. When a pgt entry's 'pres' bit
 *              is cleared, we need to flush the entry from the processor 
 *              cache so that the proceessor would use the updated software
 *              data. This is described in detail in section 4.8 of Intel IA32 
 *              software developer manual (vol 3). There are many ways to force
 *              the processor to use the software tables, than hardware cache.
 *              One such way is to reload teh CR0 register. So, if any of the
 *              'pres' bits are modified, we reload the CR0 register.
 *
 * Params:  None.
 *
 * Returns: SYSCALL
 *  OK      - on success
 *  SYSERR  - on error
 ******************************************************************************/
SYSCALL
pf_handler(void)
{
    int bs_id = EMPTY;
    int bs_offset = EMPTY;
    int fr_id = EMPTY;
    uint32_t pdir_offset = 0;
    uint32_t ptbl_offset = 0;
    uint32_t page_offset = 0;
    uint32_t fault_addr = 0;
    pd_t *pdir = NULL;
    pt_t *ptbl = NULL;
    frm_map_t *frptr = NULL;
    frm_map_t *pt_frptr = NULL;
    virt_addr_t  *fault_vaddr = NULL;
    STATWORD ps; 

    disable(ps);
    DTRACE_START;

    DTRACE("DBG$ %d %s> inside high-level page fault handler\n",    \
            currpid, __func__);

    /* vaddr   : pdir_offset : ptbl_offset : page_offset
     * 32 bits : 10 bits     : 10 bits     : 12 bits
     */
    fault_addr = read_cr2();

    /* The virtual address is 32-bits. So, we directly read the required set of 
     * bits by assigining it to a strcutre with appropriate bit-fields.
     */
    fault_vaddr = (virt_addr_t *) (&fault_addr);
    pdir_offset = (uint32_t) fault_vaddr->pd_offset;
    ptbl_offset = (uint32_t) fault_vaddr->pt_offset;
    page_offset = (uint32_t) fault_vaddr->pg_offset;
    DTRACE("DBG$ %d %s> faulted vaddr 0x%08x, vpage %d\n",  \
            currpid, __func__, fault_addr, VADDR_TO_VPAGE(fault_addr));
    DTRACE("DBG$ %d %s> pd %d, pt %d, pg %d\n",  \
            currpid, __func__, pdir_offset, ptbl_offset, page_offset);


    /* Check the BS for a mapping for the faulted vaddr and the pid. If present,
     * record the BS id and the offset within the BS. If not present, it's
     * illeagal memory access. Kill the process and return.
     */
    if (SYSERR == bsm_lookup(currpid, fault_addr, &bs_id, &bs_offset, 
                                NULL, NULL)) {
        DTRACE("DBG$ %d %s> bsm_lookup() failed\n", currpid, __func__);
        DTRACE("DBG$ %d %s> pid %d will be killed\n",   \
                currpid, __func__, currpid);
        kprintf("\n\n");
        kprintf("FATAL ERROR: Process '%s' with pid '%d' is trying to access " \
                "virtual memory out of its range! \nThe process will be "      \
                "terminated.\n", P_GET_PNAME(currpid), currpid);
        kprintf("\n\n");
        sleep(9);
        DTRACE_END;
        restore(ps);
        kill(currpid);
        goto RESTORE_AND_RETURN_ERROR;
    }   

    /* Get the currpid's page directory and index to the appropriate pgt. If pgt
     * isn't present, create one.
     */
    pdir = P_GET_PDIR(currpid);
    if (FALSE == PD_GET_PRES(pdir, pdir_offset)) {
        DTRACE("DBG$ %d %s> pgt not present for pid %d, pd offset %d, "     \
                "pt offset %d, pg offset %d, vaddr 0x%08x\n", currpid,      \
                __func__, currpid, pdir_offset, ptbl_offset, page_offset,   \
                fault_addr);
        ptbl = new_pgt();
        if (NULL == ptbl) {
            DTRACE("DBG$ %d %s> new_pgt() failed\n", currpid, __func__);
            goto RESTORE_AND_RETURN_ERROR;
        }

        /* Fill-in few meta-data for the pgt frame just created. */
        pt_frptr = FR_GET_FPTR(FR_PA_TO_ID(ptbl));
        pt_frptr->fr_pid = currpid;

        /* Set the 'pres' and 'write' bits alone. Rest would've been zeroed
         * out by new_pgt(). Also, set the base of the new page table.
         */
        pdir[pdir_offset].pd_pres = 1;
        pdir[pdir_offset].pd_write = 1;
        pdir[pdir_offset].pd_base = VADDR_TO_VPAGE((unsigned) ptbl);
    } else {
        DTRACE("DBG$ %d %s> ptbl already present at 0x%08x, fr id %d\n",  \
            currpid, __func__, VPAGE_TO_VADDR(PD_GET_BASE(pdir, pdir_offset)),\
            FR_PA_TO_ID(VPAGE_TO_VADDR(PD_GET_BASE(pdir, pdir_offset))));
    }
    ptbl = (pt_t *) VPAGE_TO_VADDR(PD_GET_BASE(pdir, pdir_offset));
    DTRACE("DBG$ %d %s> ptbl present at 0x%08x, fr id %d\n",  \
            currpid, __func__, ptbl, FR_PA_TO_ID(ptbl));

    /* Find if a frame representing the same BS id and offset is present in the
     * memory (shared pages). If so, just update the page table entry and
     * increment teh refcount.
     */
    if (EMPTY == (fr_id = is_frm_present(bs_id, bs_offset))) {
        DTRACE("DBG$ %d %s> frame not present.. creating a new frame\n",    \
                currpid, __func__);

        frptr = get_frm(FR_PAGE);
        if (NULL == frptr) {
            DTRACE("DBG$ %d %s> get_frm() failed\n", currpid, __func__);
            goto RESTORE_AND_RETURN_ERROR;
        }
        fr_id = frptr->fr_id;
        frm_pidmap_oper(fr_id, getpid(), FR_OP_PMAP_SET);
        frm_record_details(fr_id, getpid(), VADDR_TO_VPAGE(fault_addr));

        /* Read the appropriate page from BS onto the new frame. */
        if (SYSERR == read_bs((char *) FR_ID_TO_PA(fr_id), bs_id, bs_offset)) {
            DTRACE("DBG$ %d %s> read_bs() failed for fr id %d, bs %d, "     \
                    "offset %d\n", currpid, __func__, fr_id, bs_id, bs_offset);
            goto RESTORE_AND_RETURN_ERROR;
        } else {
            DTRACE("DBG$ %d %s> reading for fr id %d, bs %d, offset %d\n",  \
                    currpid, __func__, fr_id, bs_id, bs_offset);
        }

        /* Fill-in the new BS details in the frame. */
        frptr->fr_type = FR_PAGE;
        frptr->fr_bs = bs_id;
        frptr->fr_bsoffset = bs_offset;
        inc_frm_refcount(fr_id);
#ifdef DBG_ON
        print_frm_id(fr_id);
#endif
    } else {
        /* A frame representing the same BS and offset is already present in the 
         * memory. So, just increment the refcount of the frame.
         */
        frm_pidmap_oper(fr_id, getpid(), FR_OP_PMAP_SET);
        frm_record_details(fr_id, getpid(), VADDR_TO_VPAGE(fault_addr));
        inc_frm_refcount(fr_id);
    }

    /* In both cases (frame present and frame not present), we need to update
     * the page table entry as at this point, the frame is loaded onto the
     * memory. Do the following w.r.t. the pgt frame:
     *      1. Set the 'pres' bit in the page table entry corresponding to the
     *          newly loaded page to reflect that the page is present in the
     *          memory. 
     *      2. Set the 'write' bit (as given in PA3 description). 
     *      3. Update the 'base' of the page entry corresponding to the newly 
     *          created page to point to the frame.
     *      4. Increment the refcount of the pgt frame. Unlike data frames, where
     *          refocunt denotes the # of processes that map to the actual
     *          physical frame, pgt frame's refcount reflects the # of pages
     *          (that are part of this pgt) that are present in the memory. 
     *          This will be decremented when a frame is paged out and the 
     *          pgt frame will be freed when the refcount reaches zero.
     */
    ptbl[ptbl_offset].pt_pres = 1;
    ptbl[ptbl_offset].pt_write = 1;
    ptbl[ptbl_offset].pt_base = FR_ID_TO_VPAGE(fr_id);
    inc_frm_refcount(FR_PA_TO_ID(ptbl));

    /* Reload the CR0 register would force the processor to flush the tables
     * that the processor maintains in hardware cache and to use the updated
     * software tables.
     */
    enable_paging();

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

