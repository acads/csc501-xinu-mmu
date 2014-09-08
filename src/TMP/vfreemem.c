/* adhanas */
/* vfreemem.c - vfreemem */

#include <conf.h>
#include <kernel.h>
#include <mem.h>
#include <proc.h>
#include <paging.h>

/*******************************************************************************
 * Name:    vfreemem 
 *
 * Desc:    Frees the dynamically allocated vheap memory. Coalseces the free
 *          block with other free blocks, if possible.
 *
 * Params: 
 *  block   - pointer to hte starting address of the block to be freed.
 *  size    - size of the block to be freed.
 *
 * Returns: SYSCALL
 *  OK      - on success
 *  SYSERR  - on error
 ******************************************************************************/
SYSCALL	
vfreemem(struct	mblock *block, unsigned nbytes)
{
    int pid = EMPTY;
    vhlist *curr = NULL;
    vhlist *prev = NULL;
    vhlist *next = NULL;
    STATWORD ps; 

    disable(ps);
    DTRACE_START;

    pid = getpid();
    /* Error out if the pid asking for vheap memory doesn't have a vheap
     * assoiciated with it.
     */
    if (FALSE == VM_IS_VPAGE_VALID(P_GET_VPAGE(getpid()))) {
        DTRACE("DBG$ %d %s> pid %d doesn't have a vheap\n",     \
                currpid, __func__, getpid());
        goto RESTORE_AND_RETURN_ERROR;
    }   

    /* The passed addr shouldn't be NULL. */
    if (NULL == block) {
        DTRACE("DBG$ %d %s> null block addr\n", currpid, __func__);
        goto RESTORE_AND_RETURN_ERROR;
    }

    /* The given addr should be a valid vaddr. */
    if (FALSE == VADDR_IS_VALID((unsigned) block)) {
        DTRACE("DBG$ %d %s> invalid block addr %x\n", currpid, __func__, block);
        goto RESTORE_AND_RETURN_ERROR;
    }

    /* Error out if the requested bytes is more than the size of the vheap
     * associated with the proc.
     */
    if (FALSE == vh_is_nbytes_valid(getpid(), nbytes)) {
        DTRACE("DBG$ %d %s> bad nbytes %d\n", currpid, __func__, nbytes);
        goto RESTORE_AND_RETURN_ERROR;
    }   
    DTRACE("DBG$ %d %s> block 0x%08x, nbytes %d\n",     \
            currpid, __func__, (unsigned) block, nbytes);

    /* Find the appropriate block to place/merge the free block starting
     * from vhstart. 
     */
    curr = P_GET_VHEAP_START(getpid());
    while (curr && (((unsigned) curr->vloc) < ((unsigned) block))) {
        prev = curr;
        curr = (vhlist *) curr->ploc;
    }
    if (NULL == curr) {
        /* The given block is way past the vheap boundaries. This should have
         * caught earlier!
         */
        DTRACE("DBG$ %d %s> given block 0x%08x is way beyond vheap "        \
                "end 0x%08x\n", currpid, __func__, block, 
                P_GET_VHEAP_END(P_GET_BS_ID(pid)));
        goto RESTORE_AND_RETURN_ERROR;
    }
    next = (vhlist *) curr->ploc;

    DTRACE("DBG$ %d %s> prev 0x%08x, vloc 0x%08x, vlen %d, ploc 0x%08x\n",  \
            currpid, __func__, prev, prev->vloc, prev->vlen, prev->ploc);
    DTRACE("DBG$ %d %s> curr 0x%08x, vloc 0x%08x, vlen %d, ploc 0x%08x\n",  \
            currpid, __func__, curr, curr->vloc, curr->vlen, curr->ploc);
    DTRACE("DBG$ %d %s> next 0x%08x, vloc 0x%08x, vlen %d, ploc 0x%08x\n",  \
            currpid, __func__, next, next->vloc, next->vlen, next->ploc);

    /* There could be two possible cases here:
     *  1. curr == block: Simple and straight-forward case. User asked for
     *      nbytes starting at 'x' and he is now freeing the same.
     *  2. curr > block: User asked for 'nbytes' starting at 'x', but now, he
     *      is not freeing the whole block. He's freeing data of some bytes
     *      from 'x + delta'. Make sure that he's not freeing adjacent
     *      blocks. If not, create a new free block.
     */
    if (((unsigned) curr->vloc) == ((unsigned) block)) {
        DTRACE("DBG$ %d %s> curr == block\n", currpid, __func__);
        /* Check if the given nbytes overflows into either prev or next
        * blocks. If so, error out.
        */
        if (prev) {
            if ((((unsigned) prev->vloc) + nbytes) > ((unsigned) curr->vloc)) {
                DTRACE("DBG$ %d %s> given block starting at 0x%08x of " \
                        "length %d is overflowing into prev block\n",   \
                        currpid, __func__, block, nbytes);
            goto RESTORE_AND_RETURN_ERROR;
            }
        }
        if ((((unsigned) curr->vloc) + nbytes) > ((unsigned) next->vloc)) {
            DTRACE("DBG$ %d %s> given block starting at 0x%08x of length "  \
                    "%d is overflowing into next block\n",                  \
                    currpid, __func__, block, nbytes);
            goto RESTORE_AND_RETURN_ERROR;
        }

        /* The length of the current block should be 0, by design */
        if (0 != curr->vlen) {
             DTRACE("DBG$ %d %s> curr==block, but curr->vlen != 0, %d\n",   \
                     currpid, __func__, curr->vlen);
            goto RESTORE_AND_RETURN_ERROR;
        }

        /* See if we could merge the curr block with the prev or the next
         * block and free the curr block. We can do so if the prev or next 
         * block holds another free block. If neither of them do so, create 
         * a new free block at curr.
         */
        if (prev && (prev->vlen != 0) && 
                ((((unsigned) prev->vloc) + prev->vlen + nbytes) == 
                 (unsigned) next->vloc)) {
            prev->vlen += nbytes;
            prev->ploc = curr->ploc;
            freemem((struct mblock *) curr, sizeof(vhlist));
            DTRACE("DBG$ %d %s> curr == block, merging with prev block at " \
                    "0x%08x, new length of prev %d\n", currpid, __func__,   \
                    prev->vloc, prev->vlen);
            DTRACE("DBG$ %d %s> freeing ploc 0x%08x\n",                     \
                    currpid, __func__, (unsigned) curr);
        } else if ((next->vlen != 0) && 
                ((((unsigned) curr->vloc) + nbytes) == 
                 (unsigned) next->vloc)) {
            curr->vlen += (nbytes + next->vlen);
            curr->ploc = next->ploc;
            freemem((struct mblock *) next, sizeof(vhlist));
            DTRACE("DBG$ %d %s> curr == block, merging with next block at " \
                    "0x%08x, new length of next %d\n", currpid, __func__,   \
                    curr->vloc, curr->vlen);
            DTRACE("DBG$ %d %s> freeing ploc 0x%08x\n",                     \
                    currpid, __func__, (unsigned) next);
        } else {
            curr->vlen += nbytes;            
            DTRACE("DBG$ %d %s> curr == block, creating a new block at "    \
                    "0x%08x, new length %d\n", currpid, __func__,           \
                    curr->vloc, curr->vlen);
        }
    } else if (((unsigned) curr->vloc) > ((unsigned) block)) {
        DTRACE("DBG$ %d %s> curr > block\n", currpid, __func__);

        /* Check if the given nbytes overflows into either prev or next
        * blocks. If so, error out.
        */
        if ((unsigned) (((unsigned) block) + nbytes) > 
                (unsigned) curr->vloc) {
            DTRACE("DBG$ %d %s> given block starting at 0x%08x of length "  \
                    "%d is overflowing into next block\n",                  \
                    currpid, __func__, block, nbytes);
            goto RESTORE_AND_RETURN_ERROR;
        }

        /* See if we could merge the curr block with the prev or the next
         * block and free the curr block. We can do so if the prev or next 
         * block holds another free block. If neither of them do so, create 
         * a new free block at curr.
         *
         * Potential virtual memory leak here! User asked from 'n' bytes and
         * freeing only '(x - delta)' bytes!
         */
        DTRACE("DBG$ %d %s> potential vmem leak! user asked for 'nbytes' "  \
                "and freeing only a portion of it!\n", currpid, __func__);

        if (prev && (prev->vlen != 0) && 
                (((unsigned) ((unsigned) prev->vloc) + prev->vlen) ==
                 (unsigned) block)) {
            prev->vlen += nbytes;
            prev->ploc = curr->ploc;
            freemem((struct mblock *) curr, sizeof(vhlist));
            DTRACE("DBG$ %d %s> curr > block, merging with prev block at "  \
                    "0x%08x, new length of prev %d\n", currpid, __func__,   \
                    prev->vloc, prev->vlen);
            DTRACE("DBG$ %d %s> freeing ploc 0x%08x\n",                     \
                    currpid, __func__, (unsigned) curr);
        } else if (next && (next->vlen != 0) && 
                (((unsigned) ((unsigned) block) + nbytes) == 
                 ((unsigned) next->vloc))) {
            curr->vloc = (unsigned) block;
            curr->vlen += (nbytes + next->vlen);
            freemem((struct mblock *) next, sizeof(vhlist));
            DTRACE("DBG$ %d %s> curr > block, merging with next block at "  \
                    "0x%08x, new length of next %d\n", currpid, __func__,   \
                    curr->vloc, curr->vlen);
            DTRACE("DBG$ %d %s> freeing ploc 0x%08x\n",                     \
                    currpid, __func__, (unsigned) next);
        } else {
            curr->vloc = (unsigned) block;
            curr->vlen += nbytes;
            DTRACE("DBG$ %d %s> curr > block, creating a new block at "     \
                    "0x%08x, new length %d\n", currpid, __func__,           \
                    curr->vloc, curr->vlen);
        }
    }

    DTRACE("DBG$ %d %s> freed vheap memory nbytes %d starting at 0x%x\n",
            currpid, __func__, nbytes, (unsigned) block);
    DTRACE_END;
    restore(ps);
    return OK;

RESTORE_AND_RETURN_ERROR:
    DTRACE_END;
    restore(ps);
    return SYSERR;
}


