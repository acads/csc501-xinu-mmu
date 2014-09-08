/* adhanas */
/* vcreate.c - vcreate */
    
#include <conf.h>
#include <i386.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <paging.h>
#include <stdio.h>


/*************************************************************************** 
 * Name:    vcreate
 *
 * Desc:    Creates a new process with a virtual private head of size
 *          hsize pages. Uses 'create()' to actually create the process and
 *          adds a vheap to it.
 *
 * Params:
 *  procaddr    - Starting address of proc procedure.
 *  ssize       - Stack size of the proc.
 *  hsize       - virtual heap size of the proc in pages.
 *  priority    - priority of the process.
 *  name        - process name.
 *  nargs       - # of arguments to the process.
 *  args        - 1st argument; rest will be read from the stack.
 *
 * Returns: SYSCALL
 *  new pid     - on successful process creation
 *  SYSERR      - on error
 **************************************************************************/
SYSCALL 
vcreate(procaddr,ssize,hsize,priority,name,nargs,args)
	int	*procaddr;		/* procedure address		*/
	int	ssize;			/* stack size in words		*/
	int	hsize;			/* virtual heap size in pages	*/
	int	priority;		/* process priority > 0		*/
	char	*name;			/* name (for debugging)		*/
	int	nargs;			/* number of args that follow	*/
	long	args;			/* arguments (treated like an	*/
					/* array in the code)		*/
{
    int new_pid = EMPTY;
    int new_bsid = EMPTY;
    bs_map_t *bsptr = NULL;
    struct pentry *pptr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if ((hsize < 1) || (hsize > BS_MAX_NPAGES)) {
        DTRACE("DBG$ %d %s> bad hszie %d\n", currpid, __func__, hsize);
        goto RESTORE_AND_RETURN_ERROR;
    }

    /* Create the process using 'create()' and add a virtual heap to it. */
    new_pid = create(procaddr, ssize, priority, name, nargs, args);

    /* Get a new BS id for the vheap of this proc. */
    if (OK != get_bsm(&new_bsid)) {
        DTRACE("DBG$ %d %s> get_bsm() failed\n", currpid, __func__);
        goto RESTORE_AND_RETURN_ERROR;
    }

    /* Update the new BS id with required details. */
    bsptr = BS_GET_PTR(new_bsid);
    bsptr->bsm_id = new_bsid;
    bsptr->bsm_status = BS_INUSE;
    bsptr->bsm_pid = new_pid;
    bsptr->bsm_isvheap = TRUE;
    bsptr->bsm_vpno = VM_START_PAGE;
    bsptr->bsm_npages = hsize;

    /* Add a mapping between the BS and the pid. */
    if (OK != bsm_map(new_pid, bsptr->bsm_vpno, 
                bsptr->bsm_id, bsptr->bsm_npages)) {
        DTRACE("DBG$ %d %s> bsm_map() failed\n", currpid, __func__);
        goto RESTORE_AND_RETURN_ERROR;
    }

    /* Update the proc with BS specific details. */
    pptr = P_GET_PPTR(new_pid);
    pptr->store = new_bsid;
    pptr->vhpno = VM_START_PAGE;
    pptr->vhpnpages = hsize;
    pptr->bs[new_bsid] = BS_VHEAP;

    /* To facilitate the implementation of vitual heap, two pointers of type
     * vhlist will be used. They will store the vaddr location, vheap length and
     * the corresponding physical memory location of the underlying BS.
     *
     * vhstart will point to the 1st available vaddr, length set to hsize in
     * bytes and ploc will point to the vhend which is the other vhlist
     * pointer.
     */
    pptr->vhstart = (vhlist *) getmem(sizeof(vhlist));
    pptr->vhend = (vhlist *) getmem(sizeof(vhlist));

    pptr->vhstart->vloc = (uint32_t) (VM_START_PAGE * NBPG);
    pptr->vhstart->vlen = (hsize * NBPG);
    pptr->vhstart->ploc = (uint32_t *) pptr->vhend;
    DTRACE("DBG$ %d %s> vstart 0x%08x, vloc 0x%08x, vlen %d, ploc 0x%08x\n", \
            currpid, __func__, pptr->vhstart, pptr->vhstart->vloc,    \
            pptr->vhstart->vlen, pptr->vhstart->ploc);

    pptr->vhend->vloc = (uint32_t) ((VM_START_PAGE + hsize) * NBPG);
    pptr->vhend->vlen = 0;
    pptr->vhend->ploc = NULL;
    DTRACE("DBG$ %d %s> vend 0x%08x, vloc 0x%08x, vlen %d, ploc 0x%08x\n", \
            currpid, __func__, pptr->vhend, pptr->vhend->vloc,    \
            pptr->vhend->vlen, pptr->vhend->ploc);

    DTRACE("DBG$ %d %s> new proc with pid %d, vheap of size %d pages "\
            "created successfully\n", currpid, __func__, new_pid, hsize);
    DTRACE_END;
    restore(ps);
    return new_pid;

RESTORE_AND_RETURN_ERROR:
    DTRACE_END;
    restore(ps);
    return SYSERR;
}


/*************************************************************************** 
 * Name:    vh_release_vhlist 
 *
 * Desc:    Releases the vhlists if the process happens to have a vheap.
 *          This should be called only in proces exit path. 
 *
 * Params:
 *  pid     - pid of the proc whose vhlists has to be released.
 *
 * Returns: Nothing. 
 **************************************************************************/
void
vh_release_vhlist(int pid)
{
    vhlist *curr = NULL;
    vhlist *next = NULL;
    struct pentry *pptr = NULL;
    STATWORD ps;

    disable(ps);
    DTRACE_START;

    if (isbadpid(pid)) {
        DTRACE("DBG$ %d %s> bad pid %d\n", currpid, __func__, pid);
        goto RESTORE_AND_RETURN;
    }

#ifdef DBG_ON
    DTRACE("DBG$ %d %s> before releasing vhlists of pid %d..\n",    \
            currpid, __func__, pid);
    print_proc_vhlist(pid);
#endif

    /* Don't bother if the proc doesn't have a vheap. */
    if (NULL == P_GET_VHEAP_START(pid)) {
        DTRACE("DBG$ %d %s> pid %d doesn't have a vheap..\n",   \
                currpid, __func__, pid);
        goto RESTORE_AND_RETURN;
    }

    pptr = P_GET_PPTR(pid);
    next = P_GET_VHEAP_START(pid);
    while (next != NULL) {
        curr = next;
        next = (vhlist *) next->ploc;
        freemem((struct mblock *) curr, sizeof(vhlist));
    }
    pptr->vhstart = NULL;
    pptr->vhend = NULL;
    DTRACE("DBG$ %d %s> vhlists released for pid %d\n",     \
            currpid, __func__, pid);

#ifdef DBG_ON
    DTRACE("DBG$ %d %s> after releasing vhlists of pid %d..\n",    \
            currpid, __func__, pid);
    print_proc_vhlist(pid);
#endif

RESTORE_AND_RETURN:
    DTRACE_END;
    restore(ps);
    return;
}


