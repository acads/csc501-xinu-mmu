/* adhanas */
/* vgetmem.c - vgetmem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

/*******************************************************************************
* Name:     vgetmem
*
* Desc:     Allocates memory from the vheap of a process, if the process 
*           happens to have one. Callers should check if vgetmem is 
*           successful before trying to access the allocated vaddr.
*
* Params:
*   nbytes  - # of bytes to allocate
*
* Returns:
*   address - allocated vaddr on success
*   SYSERR  - on error (no vheap or no memory left)
*******************************************************************************/
WORD *	
vgetmem(unsigned nbytes)
{
    vhlist *new = NULL;
    vhlist *curr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    /* Error out if the pid asking for vheap memory doesn't have a vheap
     * assoiciated with it.
     */
    if (FALSE == VM_IS_VPAGE_VALID(P_GET_VPAGE(getpid()))) {
        DTRACE("DBG$ %d %s> pid %d doesn't have a vheap\n",     \
                currpid, __func__, getpid());
        goto RESTORE_AND_RETURN_ERROR;
    }

    /* Error out if the requested bytes is more than the size of the vheap
     * associated with the proc.
     */
    if (FALSE == vh_is_nbytes_valid(getpid(), nbytes)) {
        DTRACE("DBG$ %d %s> bad nbytes %d\n", currpid, __func__, nbytes);
        goto RESTORE_AND_RETURN_ERROR;
    }
    DTRACE("DBG$ %d %s> nbytes %d\n", currpid, __func__, nbytes);

    /* Find the first free block from vhstart. */
    curr = P_GET_VHEAP_START(getpid());
    while (curr && curr->vlen < nbytes) {
        curr = (vhlist *) curr->ploc;
    }
    if (NULL == curr) {
        DTRACE("DBG$ %d %s> no more memory left in vheap of pid %d, "   \
                "bs id %d\n", currpid, __func__, currpid, P_GET_BS_ID(currpid));
        goto RESTORE_AND_RETURN_ERROR;
    }
    DTRACE("DBG$ %d %s> curr vloc 0x%08x, vlen %d, ploc 0x%08x\n",  \
            currpid, __func__, curr->vloc, curr->vlen, curr->ploc);

    /* Create a new vhlist node to point to the next available memory 
     * location. 
     */
    new = (vhlist *) getmem(sizeof(vhlist));
    if ((int *) SYSERR == (int *) new) {
        DTRACE("DBG$ %d %s> getmem failed\n", currpid, __func__);
        goto RESTORE_AND_RETURN_ERROR;
    }

    /* If we are here, the new request for 'nbytes' can be satisfied/ Update
     * the pointers to reflect this.
     */
    new->ploc = curr->ploc;
    new->vlen = ((curr->vlen) - nbytes);
    new->vloc = ((curr->vloc) + nbytes);
    curr->ploc = (uint32_t *) new;
    curr->vlen = 0;

    DTRACE("DBG$ %d %s> allocating %d bytes of vheap memory starting "  \
            "at 0x%08x\n", currpid, __func__, nbytes, curr->vloc);
    DTRACE("DBG$ %d %s> next available free memory is at ploc 0x%08x, " \
            "vloc 0x%08x of size %d bytes\n", currpid, __func__, new,   \
            new->vloc, new->vlen);

    DTRACE_END;
    restore(ps);
    return ((WORD *) curr->vloc);

RESTORE_AND_RETURN_ERROR:
    DTRACE_END;
    restore(ps); 
    return ((WORD *) SYSERR);
}


/*******************************************************************************
* Name:     vh_is_nbytes_valid
*
* Desc:     Checks whether the given nbytes is valid request for a vgetmem for
*           a proc.
*
* Params:
*   pid     - pid of the proc requesting vheap memory
*   nbytes  - # of bytes to allocate
*
* Returns:  uint8_t
*   TRUE    - if the requested nbytes is valid
*   FALSE   - otherwise
*******************************************************************************/
uint8_t
vh_is_nbytes_valid(int pid, int nbytes)
{
    int bs_nbytes = 0;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (isbadpid(pid)) {
        DTRACE("DBG$ %d %s> bad pid %d\n", currpid, __func__, pid);
        goto RESTORE_AND_RETURN_FALSE;
    }

    if (0 == nbytes) {
        DTRACE("DBG$ %d %s> bad nbytes %d\n", currpid, __func__, nbytes);
        goto RESTORE_AND_RETURN_FALSE;
    }

    bs_nbytes = (NBPG * P_GET_NUM_VPAGES(pid));
    if (nbytes > bs_nbytes) {
        DTRACE("DBG$ %d %s> bad nbytes %d\n", currpid, __func__, nbytes);
        goto RESTORE_AND_RETURN_FALSE;
    }

    DTRACE("DBG$ %d %s> returning TRUE\n", currpid, __func__);
    DTRACE_END;
    restore(ps);
    return TRUE;

RESTORE_AND_RETURN_FALSE:
    DTRACE("DBG$ %d %s> returning FALSE\n", currpid, __func__);
    DTRACE_END;
    restore(ps);
    return FALSE;
}

