/* adhanas */
/* PA3 - paging print utils */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <stdio.h>
#include <paging.h>

#ifdef DBG_ON

/* Printable BS states */
const char *bs_state_str[] = {
    "Free",
    "In use",
    "Unmapped",
    "Mapped"
};

const char *bs_vheap_str[] = {
    "No",
    "Yes"
};

const char *fr_state_str[] = {
    "Free",
    "In Use"
};

const char *fr_type_str[] = {
    "Data page",
    "Table page",
    "Dir page"
};

#define BS_GET_STATE_STR(bs_id) (bs_state_str[BS_GET_STATE(bs_id) - 1])
#define BS_GET_VHEAP_STR(bs_id) (bs_vheap_str[BS_IS_VHEAP(bs_id)])

#define FR_GET_STATE_STR(fr_id) (fr_state_str[FR_GET_STATUS(fr_id) - 1])
#define FR_GET_TYPE_STR(fr_id)  (fr_type_str[FR_GET_TYPE(fr_id) - 1])

/******************************************************************************
 * Name:    print_bsm_tab 
 *
 * Desc:    Prints the contents of BS tables.
 *
 * Params: 
 *  detail_flag - detailed info flag
 *
 * Returns: Nothing
 *****************************************************************************/
void
print_bsm_tab(uint8_t detail_flag)
{
    int bs_id = 0;
    uint8_t i = 0;
    bs_map_t *bsptr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    kprintf("\n");
    kprintf("*** BS TAB INFORMATION START ***\n", NULL);
    for (bs_id = 0; bs_id < BS_NUM; ++bs_id) {
        bsptr = BS_GET_PTR(bs_id);
        kprintf("BS id: %d\n", bsptr->bsm_id);
        kprintf("State: %s\n", BS_GET_STATE_STR(bs_id));
        kprintf("VHeap: %s\n", BS_GET_VHEAP_STR(bs_id));
        kprintf("Total maps: %d\n", BS_GET_COUNT(bs_id));
        if (detail_flag) {
            for (i = 0; i < BS_GET_COUNT(bs_id); ++i) {
                kprintf("   PID: %d\n", bsptr->bsm_pid);
                kprintf("   VPage: %d\n", bsptr->bsm_vpno);
                kprintf("   NPages: %d\n", bsptr->bsm_npages);
                bsptr = bsptr->bsm_next;
            }
        }
    }
    kprintf("*** BS TAB INFORMATION END ***\n", NULL);
    kprintf("\n");

    DTRACE_END;
    restore(ps);
    return;
}

/******************************************************************************
 * Name:    print_nonfree_bsm_tab 
 *
 * Desc:    Prints the contents of BS tables that are not free.
 *
 * Params: 
 *  detail_flag - detailed info flag
 *
 * Returns: Nothing
 *****************************************************************************/
void
print_nonfree_bsm_tab(uint8_t detail_flag)
{
    int bs_id = 0;
    bs_map_t *bsptr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    kprintf("\n");
    kprintf("*** BS TAB (non-free) INFORMATION START ***\n", NULL);
    for (bs_id = 0; bs_id < BS_NUM; ++bs_id) {
        bsptr = BS_GET_PTR(bs_id);
        if (BS_FREE == bsptr->bsm_status)
            continue;

        kprintf("BS id: %d\n", bsptr->bsm_id);
        kprintf("State: %s\n", BS_GET_STATE_STR(bs_id));
        kprintf("VHeap: %s\n", BS_GET_VHEAP_STR(bs_id));
        kprintf("Total maps: %d\n", BS_GET_COUNT(bs_id));
        if (detail_flag) {
            //for (i = 0; i < BS_GET_COUNT(bs_id); ++i) {
            while (bsptr) {
                kprintf("   PID: %d\n", bsptr->bsm_pid);
                kprintf("   VPage: %d\n", bsptr->bsm_vpno);
                kprintf("   NPages: %d\n", bsptr->bsm_npages);
                bsptr = bsptr->bsm_next;
            }
        }
    }
    kprintf("*** BS TAB (non-free) iINFORMATION END ***\n", NULL);
    kprintf("\n");

    DTRACE_END;
    restore(ps);
    return;
}


/******************************************************************************
 * Name:    print_bs_details
 *
 * Desc:    Prints details of the given BS id.
 *
 * Params: 
 *  bs_id       - details of the BS to be printed
 *  detail_flag - detailed information flag
 *
 * Returns: Nothing
 *****************************************************************************/
void
print_bs_details(int bs_id, uint8_t detail_flag)
{
    bs_map_t *bsptr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (FALSE == BS_IS_ID_VALID(bs_id)) {
        kprintf("DBG$ %d %s> bad bs id %d\n", currpid, __func__, bs_id);
        goto RESTORE_AND_RETURN;
    }
    bsptr = BS_GET_PTR(bs_id);

    kprintf("\n");
    kprintf("*** BS INFORMATION START ***\n", NULL);
    kprintf("BS id: %d\n", bs_id);
    kprintf("State: %s\n", BS_GET_STATE_STR(bs_id));
    kprintf("VHeap: %s\n", BS_GET_VHEAP_STR(bs_id));
    kprintf("Total maps: %d\n", BS_GET_COUNT(bs_id));
    if (detail_flag) {
        while (bsptr && BS_GET_COUNT(bs_id)) {
            kprintf("   PID: %d\n", bsptr->bsm_pid);
            kprintf("   VPage: %d\n", bsptr->bsm_vpno);
            kprintf("   NPages: %d\n", bsptr->bsm_npages);
            bsptr = bsptr->bsm_next;
        }
    }
    kprintf("*** BS INFORMATION END ***\n", NULL);
    kprintf("\n");

RESTORE_AND_RETURN:
    DTRACE_END;
    restore(ps);
    return;
}


/******************************************************************************
 * Name:    print_frm_data
 *
 * Desc:    Prints debug data for frames.
 *
 * Params:  None. 
 *
 * Returns: Nothing.
 *****************************************************************************/
void
print_frm_data(void)
{
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    kprintf("\n");
    kprintf("*** FRAME DEBUG DATA START ***\n");
    kprintf("# of free frames: %d\n", frm_data.fr_nfree);
    kprintf("# of used frames: %d\n", frm_data.fr_nused);
    kprintf("# of page frames: %d\n", frm_data.fr_pages);
    kprintf("# of pt frames: %d\n", frm_data.fr_tables);
    kprintf("# of pd frames: %d\n", frm_data.fr_dirs);
    kprintf("*** FRAME DEBUG DATA END ***\n");
    kprintf("\n");
    
    DTRACE_END;
    restore(ps);
    return;
}

/******************************************************************************
 * Name:    print_frm_id
 *
 * Desc:    Prints frame details.
 *
 * Params:  
 *  fr_id   - id of the frame to be printed 
 *
 * Returns: Nothing.
 *****************************************************************************/
void
print_frm_id(int fr_id)
{
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (FALSE == FR_IS_ID_VALID(fr_id)) {
        kprintf("ERR: bad fr id %d\n", fr_id);
        goto END;
    }

    kprintf("\n");
    kprintf("*** FRAME INFORMATION START ***\n");

    kprintf("Frame ID     : %d\n", fr_id);
    kprintf("Frame status : %s\n", FR_GET_STATE_STR(fr_id));
    kprintf("Frame PID    : %d\n", FR_GET_PID(fr_id));
    //kprintf("Frame vpage  : %d\n", FR_GET_VPAGE(fr_id, pid));
    kprintf("Frame refcnt : %d\n", FR_GET_RCOUNT(fr_id));
    kprintf("Frame type   : %s\n", FR_GET_TYPE_STR(fr_id));
    kprintf("Frame BS     : %d\n", FR_GET_BS(fr_id));
    kprintf("Frame offset : %d\n", FR_GET_BSOFFSET(fr_id));
    kprintf("Frame ltime  : %d\n", FR_GET_LTIME(fr_id));

    kprintf("*** FRAME INFORMATION END ***\n");
    kprintf("\n");
    
END:
    DTRACE_END;
    restore(ps);
    return;
}

/******************************************************************************
 * Name:    print_pdbr
 *
 * Desc:    Prints pdbr values for all active processes or the given process.
 *
 * Params:
 *  given_pid   - pid whose PDBR value is to be printed.
 *
 * Returns: Nothing.
 *****************************************************************************/
void
print_pdbr(int given_pid)
{
    int pid = 0;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    kprintf("\n");
    kprintf("*** PID PDBR VALUE START ***\n");
    if (EMPTY == given_pid) {
        for (pid = 0; pid < NPROC; ++pid) {
            if (PRFREE == P_GET_PSTATE(pid)) {
                continue;
            }
            kprintf("%d   0x%08x\n", pid, P_GET_PDIR(pid));
        }
    } else {
        if (PRFREE == P_GET_PSTATE(given_pid)) {
            kprintf("givn pid %d is free!\n");
        } else {
            kprintf("%d   0x%08x\n", pid, P_GET_PDIR(pid));
        }
    }
    kprintf("*** PID PDBR VALUE END ***\n");
    kprintf("\n");
    DTRACE_END;
    restore(ps);
    return;
}


/******************************************************************************
 * Name:    print_proc_vhlist
 *
 * Desc:    Prints the procs vhlist 
 *
 * Params:
 *  pid   - pid whose vhlist is to be printed
 *
 * Returns: Nothing.
 *****************************************************************************/
void
print_proc_vhlist(int pid)
{
    vhlist *start = NULL;
    STATWORD ps;

    if (isbadpid(pid))
        return;

    disable(ps);
    DTRACE_START;

    kprintf("\n");
    kprintf("*** PID VLIST START ***\n");

    start = P_GET_VHEAP_START(pid);
    while (NULL != start) {
        kprintf("Nloc: 0x%08x\n", start);
        kprintf("VLoc: 0x%08x\n", start->vloc);
        kprintf("VLen: %d\n", start->vlen);
        kprintf("PLoc: 0x%08x\n\n", start->ploc);

        start = (vhlist *) start->ploc;
    }

    kprintf("*** PID VLIST END ***\n");
    kprintf("\n");

    DTRACE_END;
    restore(ps);
    return;
}

/******************************************************************************
 * Name:    print_sc_tab
 *
 * Desc:    Prints the SC table.
 *
 * Params:  None.
 *
 * Returns: Nothing.
 *****************************************************************************/
void
print_sc_tab(void)
{
    int i = 0;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    kprintf("\n");
    kprintf("*** SC TABLE INFORMATION START ***\n");

    kprintf("sc iter is at %d\n", sc_iter);
    kprintf("POS\t FRAME\n");
    for (i = 0; i < FR_NFRAMES; ++i) {
        kprintf("%d\t %d\n", i, sc_tab[i]);
    }

    kprintf("*** SC TABLE INFORMATION END ***\n");
    kprintf("\n");

    DTRACE_END;
    return;
}


/******************************************************************************
 * Name:    print_nru_tabs
 *
 * Desc:    Prints the NRU tables.
 *
 * Params:  None.
 *
 * Returns: Nothing.
 *****************************************************************************/
void
print_nru_tabs(void)
{
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    kprintf("\n");
    kprintf("*** NRU TABLES INFORMATION START ***\n");

    kprintf("# of frames in C0: %d\n", PR_NRU_GET_C0_CTR);
    print_nru_tab(nru_c0_tab);

    kprintf("# of frames in C1: %d\n", PR_NRU_GET_C1_CTR);
    print_nru_tab(nru_c1_tab);

    kprintf("# of frames in C2: %d\n", PR_NRU_GET_C2_CTR);
    print_nru_tab(nru_c2_tab);

    kprintf("# of frames in C3: %d\n", PR_NRU_GET_C3_CTR);
    print_nru_tab(nru_c3_tab);

    kprintf("*** NRU TABLES INFORMATION END ***\n");
    kprintf("\n");

    DTRACE_END;
    restore(ps);
    return;
}


/******************************************************************************
 * Name:    print_nru_tab
 *
 * Desc:    Prints the NRU tables.
 *
 * Params:
 *  nru_tab - pointer to any one of the 4 NRU tables
 *
 * Returns: Nothing.
 *****************************************************************************/
void
print_nru_tab(int *nru_tab)
{
    int fr_id = 0;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    kprintf("POS\t FRAME \n");
    for (fr_id = 0; fr_id < FR_NFRAMES; ++fr_id) {
        if (EMPTY == nru_tab[fr_id]) {
            continue;
        }

        if (FR_PAGE != FR_GET_TYPE(nru_tab[fr_id])) {
            continue;
        }

        kprintf("%d\t %d \n", fr_id, nru_tab[fr_id]);
    }
    kprintf("\n");

    DTRACE_END;
    restore(ps);
    return;
}

#endif /* DBG_ON */

