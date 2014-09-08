/* adhanas */
/* paging.h */

#ifndef _PAGING_H_
#define _PAGING_H_

/*----------------------------- DAN UTILS START -----------------------------*/
#define uint8_t     unsigned char
#define uint32_t    unsigned int
#define boolean     unsigned char

#if 0
#define TRUE        1
#define FALSE       2
#endif

/* Debug trace utils */
#ifdef DBG_ON
#define DTRACE(STR, ...)    kprintf(STR, __VA_ARGS__)
#define DTRACE_START        kprintf("DBG$ %d %s> start\n", currpid, __func__)
#define DTRACE_END          kprintf("DBG$ %d %s> end\n", currpid, __func__)
//#define ASSERT(BOOL)        if (0 == BOOL) trap(17)
#else
#define DTRACE
#define DTRACE_START
#define DTRACE_END
//#define ASSERT
#endif /* DBG_ON */

/* PID related macros */
#define P_GET_PID(PID)          (proctab[PID].pid)
#define P_GET_PNAME(PID)        (proctab[PID].pname)
#define P_GET_PPTR(PID)         (&proctab[PID])
#define P_GET_PPRIO(PID)        (proctab[PID].pprio)
#define P_GET_PSTATE(PID)       (proctab[PID].pstate)
#define P_GET_PSTATESTR(PID)    (l_pstates_str[proctab[PID].pstate - 1])
#define P_GET_BS_ID(PID)        (proctab[PID].store)
#define P_GET_VPAGE(PID)        (proctab[PID].vhpno)
#define P_GET_NUM_VPAGES(PID)   (proctab[PID].vhpnpages)
#define P_GET_VHEAP(PID)        (&proctab[PID].vmemlist)
#define P_GET_VHEAP_START(PID)  (proctab[PID].vhstart)
#define P_GET_VHEAP_END(PID)    (proctab[PID].vhend)
#define P_GET_PDIR(PID)         (proctab[PID].pgdir)
/*------------------------------ DAN UTILS END ------------------------------*/

/*------------------------------- PAGES START -------------------------------*/
/* Structure for a page directory entry */
typedef struct {
  unsigned int pd_pres	: 1;		/* page table present?		*/
  unsigned int pd_write : 1;		/* page is writable?		*/
  unsigned int pd_user	: 1;		/* is use level protection?	*/
  unsigned int pd_pwt	: 1;		/* write through cachine for pt?*/
  unsigned int pd_pcd	: 1;		/* cache disable for this pt?	*/
  unsigned int pd_acc	: 1;		/* page table was accessed?	*/
  unsigned int pd_mbz	: 1;		/* must be zero			*/
  unsigned int pd_fmb	: 1;		/* four MB pages?		*/
  unsigned int pd_global: 1;		/* global (ignored)		*/
  unsigned int pd_avail : 3;		/* for programmer's use		*/
  unsigned int pd_base	: 20;		/* location of page table?	*/
} pd_t;


/* Structure for a page table entry */
typedef struct {
  unsigned int pt_pres	: 1;		/* page is present?		*/
  unsigned int pt_write : 1;		/* page is writable?		*/
  unsigned int pt_user	: 1;		/* is use level protection?	*/
  unsigned int pt_pwt	: 1;		/* write through for this page? */
  unsigned int pt_pcd	: 1;		/* cache disable for this page? */
  unsigned int pt_acc	: 1;		/* page was accessed?		*/
  unsigned int pt_dirty : 1;		/* page was written?		*/
  unsigned int pt_mbz	: 1;		/* must be zero			*/
  unsigned int pt_global: 1;		/* should be zero in 586	*/
  unsigned int pt_avail : 3;		/* for programmer's use		*/
  unsigned int pt_base	: 20;		/* location of page?		*/
} pt_t;

/* Constants */
#define PT_NGPT         4
#define PT_UNIT_SIZE    4096
/* Each entry is 4 bytes long. So, each frame (NBPG bytes) can hold
 * NBPG/4 PT entries.
 */
#define PT_PER_FRAME    (NBPG / 4)

/* Page table/directory operations. */
#define PT_OP_ACC_BIT_CHK       1
#define PT_OP_ACC_BIT_CLR       2
#define PT_OP_DIRTY_BIT_CHK     3
#define PT_OP_PTPRES_BIT_CHK    4
#define PT_OP_PTPRES_BIT_CLR    5
#define PT_OP_PDPRES_BIT_CHK    6
#define PT_OP_PDPRES_BIT_CLR    7
#define PT_OP_INC_RCOUNT        8
#define PT_OP_DEC_RCOUNT        9
#define PD_OP_INC_RCOUNT        10
#define PD_OP_DEC_RCOUNT        11
#define PT_OP_FREE_FRM          12

/* Utils */
#define PD_GET_PRES(pdir, pt_id)    (pdir[pt_id].pd_pres)
#define PD_GET_BASE(pdir, pt_id)    (pdir[pt_id].pd_base)
#define PT_GET_PRES(ptbl, pg_id)    (ptbl[pg_id].pt_pres)
#define PT_GET_BASE(ptbl, pg_id)    (ptbl[pg_id].pt_base)
#define PT_GET_DIRTY(ptbl, pg_id)   (ptbl[pg_id].pt_dirty)
#define PT_GET_ACC(ptbl, pg_id)     (ptbl[pg_id].pt_acc)

/* Externs */
extern pt_t *g_pt[];
extern void init_pgt(pt_t *ptptr);
extern void init_global_pgt(void);
extern pt_t* new_pgt(void);
extern pd_t* new_pgd(void);
extern void remove_pgd(int pgd);
extern int pgt_update_all(int fr_id);
extern int pgt_update_for_pid(int fr_id, int pid);
extern int pgt_oper(int fr_id, int pid, int vpage, int oper);
extern SYSCALL pf_handler(void);
extern SYSCALL pf_interrupt(void);

extern unsigned long read_cr2(void);
extern void set_pdbr(unsigned pd_addr);
extern void enable_paging(void);
/*------------------------------- PAGES END -------------------------------*/


/*------------------------------- FRAMES START -------------------------------*/
/* Frame schema */
typedef struct {
    int fr_id;                  /* frame ID                     */
    int fr_status;		/* free or being used?  	*/
    int fr_pid;			/* process id using this frame  */
    int fr_vpage[NPROC];	/* corresponding vpage          */
    int fr_refcnt;	        /* reference count		*/
    int fr_type;		/* FR_DIR, FR_TBL, FR_PAGE	*/
    int fr_dirty;               /* dirty or not?                */
    int fr_bs;                  /* associated BS                */
    int fr_bsoffset;            /* page offset within BS        */
    unsigned char fr_pidmap[7]; /* pidmap of this frame         */
    unsigned long int fr_ltime;	/* when the page is loaded 	*/
    //void *fr_cookie;		/* private data structure	*/
} frm_map_t;

/* Frame debug data */
typedef struct {
    int fr_nfree;                       /* # free frames                */
    int fr_nused;                       /* # used frames                */
    int fr_pages;                       /* # of pages                   */
    int fr_tables;                      /* # of frames for tables       */
    int fr_dirs;                        /* # of frames for dirs         */
} frm_data_t;


/* Frames */
#define FR_NFRAMES      NFRAMES
#define FR_FRAME0       FRAME0
#define NBPG		4096	/* number of bytes per page	*/
//#define FR_FRAME0	1024	/* zero-th frame		*/
//#define FR_NFRAMES 	1024	/* number of frames		*/
#define FR_TBL_LIMIT    512
#define FRAME0          1024
#define NFRAMES         1024

/* Frame status */
#define FR_FREE	        1
#define FR_INUSE	2

/* Frame type */
#define FR_PAGE		1
#define FR_PTBL		2
#define FR_PDIR		3

/* Frame pidmap operations */
#define FR_OP_PMAP_SET      1
#define FR_OP_PMAP_CLR      2
#define FR_OP_PMAP_CHK      3

/* Frame utils */
#define FR_GET_FPTR(f)          (&frm_tab[f])
#define FR_GET_ID(f)            (frm_tab[f].fr_id)
#define FR_GET_STATUS(f)        (frm_tab[f].fr_status)
#define FR_GET_PID(f)           (frm_tab[f].fr_pid)
#define FR_GET_VPAGE(f)         (frm_tab[f].fr_vpno)
#define FR_GET_RCOUNT(f)        (frm_tab[f].fr_refcnt)
#define FR_GET_TYPE(f)          (frm_tab[f].fr_type)
#define FR_GET_DIRTY(f)         (frm_tab[f].fr_dirty)
#define FR_GET_BS(f)            (frm_tab[f].fr_bs)
#define FR_GET_BSOFFSET(f)      (frm_tab[f].fr_bsoffset)
#define FR_GET_LTIME(f)         (frm_tab[f].fr_ltime)
#define FR_INC_REFCOUNT(f)      (frm_tab[f].fr_refcnt += 1)
#define FR_DEC_REFCOUNT(f)      (frm_tab[f].fr_refcnt -= 1)

#define FR_GET_NFREE            (frm_data.fr_nfree)
#define FR_GET_NUSED            (frm_data.fr_nused)
#define FR_GET_NPAGES           (frm_data.fr_npages)
#define FR_GET_NTABLES          (frm_data.fr_ntables)
#define FR_GET_NDIRS            (frm_data.fr_ndirs)

#define FR_INC_NFREE            (frm_data.fr_nfree += 1)
#define FR_INC_NUSED            (frm_data.fr_nused += 1)
#define FR_INC_NPAGES           (frm_data.fr_pages += 1)
#define FR_INC_NTABLES          (frm_data.fr_tables += 1)
#define FR_INC_NDIRS            (frm_data.fr_dirs += 1)
#define FR_DEC_NFREE            (frm_data.fr_nfree -= 1)
#define FR_DEC_NUSED            (frm_data.fr_nused -= 1)
#define FR_DEC_NPAGES           (frm_data.fr_pages -= 1)
#define FR_DEC_NTABLES          (frm_data.fr_tables -= 1)
#define FR_DEC_NDIRS            (frm_data.fr_dirs -= 1)

#define FR_IS_ID_VALID(f)       ((f >= 0) && (f < FR_NFRAMES))
#define FR_IS_VALID_TYPE(ftype) ((FR_PAGE == ftype) || (FR_PTBL == ftype) || \
                                    (FR_PDIR == ftype))
#define FR_IS_VALID_BSOFFSET(offset)    ((offset >= 0) &&   \
                                            (offset < BS_MAX_NPAGES))

/* Frame 0 starts at 1024th page. So, the PA of frame 'i' can be obtained by
 * adding the base frame (1024) to the 'i'th frame and multiplying it by NBPG.
 */
#define FR_ID_TO_PA(f)          ((FR_FRAME0 + f) * NBPG)
#define FR_PA_TO_ID(pa)         ((((unsigned) pa) / NBPG) - FR_FRAME0)
#define FR_ID_TO_VPAGE(f)       (((FR_ID_TO_PA(f)) / NBPG))
#define FR_VPAGE_TO_ID(vpage)   (FR_PA_TO_ID(VPAGE_TO_VADDR(vpage)))


/* Frame externs */
extern frm_map_t frm_tab[];
extern frm_data_t frm_data;

extern void init_frm_tab(void);
extern void init_frm(int fr_id);
extern frm_map_t* get_frm(int fr_type);
extern SYSCALL free_frm(int frm);
extern frm_map_t *evict_frm(int fr_type);
extern frm_map_t *evict_frm_sc(void);
extern frm_map_t *evict_frm_nru(void);
extern int is_frm_present(int bs_id, int offset);
extern void inc_frm_refcount(int fr_id);
extern void dec_frm_refcount(int fr_id);
extern int frm_pidmap_oper(int fr_id, int pid, int oper);
extern void frm_record_details(int fr_id, int pid, int vpage);
/*------------------------------- FRAMES END -------------------------------*/


/*------------------------------- POLICIES START -------------------------------*/
/* Comstants */
#define SC		3
#define NRU		4

#define PR_NRU_MAX_LTIME    4294967295
#define PR_NRU_GET_C0_CTR   nru_c0_ctr
#define PR_NRU_GET_C1_CTR   nru_c1_ctr
#define PR_NRU_GET_C2_CTR   nru_c2_ctr
#define PR_NRU_GET_C3_CTR   nru_c3_ctr

#define PR_ACC_BIT      1
#define PR_DIRTY_BIT    2

#define PR_NRU_TICKS    2000

/* Utils */
#define PR_NRU_INC_C0_CTR (++nru_c0_ctr)
#define PR_NRU_INC_C1_CTR (++nru_c1_ctr)
#define PR_NRU_INC_C2_CTR (++nru_c2_ctr)
#define PR_NRU_INC_C3_CTR (++nru_c3_ctr)
#define PR_NRU_DEC_C0_CTR (--nru_c0_ctr)
#define PR_NRU_DEC_C1_CTR (--nru_c1_ctr)
#define PR_NRU_DEC_C2_CTR (--nru_c2_ctr)
#define PR_NRU_DEC_C3_CTR (--nru_c3_ctr)

/* Externs */
extern unsigned long ctr1000;
extern int page_replace_policy;

extern int sc_tab[];
extern int sc_iter;

extern int nru_c0_ctr;
extern int nru_c1_ctr;
extern int nru_c2_ctr;
extern int nru_c3_ctr;
extern int pr_nru_ticks;
extern int nru_c0_tab[];
extern int nru_c1_tab[];
extern int nru_c2_tab[];
extern int nru_c3_tab[];

extern SYSCALL srpolicy(int policy);
extern int grpolicy(void);

extern void sc_init_tab(int sysinit);
extern void sc_build_tab(void);
extern void sc_inc_iter(void);
extern void sc_insert_frm(int fr_id);
extern int sc_evict_frm(void);
extern int sc_is_acc_bit_set(int fr_id);
extern void sc_clr_acc_bit(int fr_id);

extern void nru_init_tab(int sysinit);
extern void nru_build_tab(void);
extern void nru_insert_frm(int fr_id, int acc_bit, int dirty_bit);
extern int  nru_is_bit_set(int fr_id, int bit_type);
extern void nru_clr_acc_bit(void);
extern int  nru_evict_frm(void);
extern int  nru_get_oldest_frm(int *nru_tab);

/*------------------------------- POLICIES END -------------------------------*/


/*---------------------------------- BS START --------------------------------*/
typedef unsigned int	 bsd_t;

/* BS constants. */
#define MAX_ID          15              
#define BS_NUM          16
#define BS_MAX_NPAGES   128

#define BS_FREE         1
#define BS_INUSE        2
#define BS_UNMAPPED	3
#define BS_MAPPED	4
#define BS_VHEAP        5

#define BS_POS_HEAD     1
#define BS_POS_MID      2
#define BS_POS_TAIL     3

#define BS_BASE	                    0x00800000
#define BS_UNIT_SIZE                0x00080000

/* BS utils. */
#define BS_TO_PA(bs_id)             (BS_BASE + (bs_id * BS_UNIT_SIZE))
#define BS_GET_PTR(bs_id)           (bsm_tab[bs_id])
#define BS_GET_STATE(bs_id)         (bsm_tab[bs_id]->bsm_status)
#define BS_GET_TPTR(bs_id)          (bsm_data.bsm_tail[bs_id])
#define BS_GET_COUNT(bs_id)         (bsm_data.count[bs_id])
#define BS_GET_PID(bs_id)           (bsm_tab[bs_id]->bsm_pid)
#define BS_GET_VPAGE(bs_id)         (bsm_tab[bs_id]->bsm_vpno)
#define BS_GET_NPAGES(bs_id)        (bsm_tab[bs_id]->bsm_npages)
//#define BS_GET_OFFSET_FOR_VPAGE(bs_id, vpage)   (vpage % NBPG)
#define BS_GET_OFFSET_FOR_VPAGE(vpage, bsvpage)   (vpage - bsvpage)
//#define BS_GET_OFFSET_FOR_VPAGE(bs_id, vpage)   (vpage - BS_GET_VPAGE(bs_id))
#define BSPTR_GET_OFFSET_FOR_VPAGE(bsptr, vpage)    \
    (vpage - bsptr->bsm_vpno)

#define BS_IS_ID_VALID(bs_id)       ((bs_id >= 0) && (bs_id <= MAX_ID))
#define BS_IS_NPAGES_VALID(npages)  ((npages > 0) && (npages <= BS_MAX_NPAGES))
#define BS_IS_FREE(bs_id)           (BS_FREE == bsm_tab[bs_id]->bsm_status)
#define BS_IS_VHEAP(bs_id)          (bsm_tab[bs_id]->bsm_isvheap)
#define BS_IS_PID_VALID(bs_id, pid) ((!BS_IS_FREE(bs_id)) &&    \
                                     (pid == bsm_tab[bs_id]->bsm_pid))
#define BS_IS_VPAGE_VALID(bs_id, vpage)                         \
    ((vpage >= bsm_tab[bs_id]->bsm_vpno) &&                     \
     (vpage < (bsm_tab[bs_id]->bsm_vpno + bsm_tab[bs_id]->bsm_npages)))
#define BSPTR_IS_VPAGE_VALID(bsptr, vpage)                      \
    ((vpage >= bsptr->bsm_vpno) &&                              \
     (vpage < (bsptr->bsm_vpno + bsptr->bsm_npages)))

#define BS_INC_NPAGES(bs_id, npg)   (bsm_tab[bs_id]->bsm_npages += npg)
#define BS_SET_COUNT(bs_id, n)      (bsm_data.count[bs_id] = n)
#define BS_INC_COUNT(bs_id)         ((bsm_data.count[bs_id])++)
#define BS_DEC_COUNT(bs_id)         ((bsm_data.count[bs_id])--)
#define BS_INC_UNMAP_COUNT(bs_id)   ((bsm_tab[bs_id]->bsm_unmap_count++))
#define BS_INC_UNMAP_COUNT(bs_id)   ((bsm_tab[bs_id]->bsm_unmap_count++))
#define BS_DEC_REL_COUNT(bs_id)     ((bsm_tab[bs_id]->bsm_unmap_count--))
#define BS_DEC_REL_COUNT(bs_id)     ((bsm_tab[bs_id]->bsm_unmap_count--))


/* Backing store maps */
typedef struct bs_map_t__ {
    bsd_t bsm_id;                   /* map belongs to this BS       */
    int bsm_status;	            /* MAPPED or UNMAPPED	    */
    int bsm_pid;	            /* process id using this slot   */
    int bsm_isvheap;                /* vheap or not                 */
    int bsm_vpno;	            /* starting virtual page number */
    int bsm_npages;	            /* number of pages in the store */
    int bsm_sem;	            /* semaphore mechanism ?	    */
    int bsm_unmap_count;
    int bsm_release_count;
    struct bs_map_t__ *bsm_next;    /* further entries in the map   */
} bs_map_t;


/* BS supplemental data */
typedef struct {
    bs_map_t    *bsm_tail[16];      /* tail pointers of the map     */
    uint8_t     count[16];          /* # of pids mapped to a map    */
} bs_data_t; 


/* Externs. */
extern bs_map_t *bsm_tab[];
extern bs_data_t bsm_data;

extern int get_bs(bsd_t, unsigned int);
extern SYSCALL release_bs(bsd_t);
extern SYSCALL read_bs(char *, bsd_t, int);
extern SYSCALL write_bs(char *, bsd_t, int);
extern SYSCALL xmmap(int vpage, bsd_t bs_id, int npages);
extern SYSCALL xmunmap(int vpage);

extern SYSCALL init_bsm(void);
extern SYSCALL get_bsm(int *avail);
extern SYSCALL free_bsm(int bs_id);
extern SYSCALL bsm_lookup(int pid, long vaddr, int* store, int* pageth,
                                 bs_map_t **prevmap, uint8_t *ishead);
extern SYSCALL bsm_map(int pid, int vpno, int bs_id, int npages);
extern SYSCALL bsm_unmap(int pid, int vpage, int flag);
extern SYSCALL bsm_remove_frm(int pid, bs_map_t *bsptr);
extern SYSCALL bsm_remove_proc_maps(int pid);
extern bs_map_t* bsm_get_ptr_for_pid(int bs_id, int pid);
extern SYSCALL bsm_handle_proc_kill(int pid);
/*----------------------------------- BS END ---------------------------------*/


/*------------------------------------ VM START ---------------------------------*/
/* Virtual address */
typedef struct{
  unsigned int pg_offset : 12;		/* page offset			*/
  unsigned int pt_offset : 10;		/* page table offset		*/
  unsigned int pd_offset : 10;		/* page directory offset	*/
} virt_addr_t;


/* Vheap memlist */
typedef struct __vhlist {
    uint32_t    vloc;   /* contains vaddr of the vheap                      */
    uint32_t    vlen;   /* contains the length of available vaddr in vheap  */
    uint32_t    *ploc;  /* points to the PM of the corresponding vloc in BS */
} vhlist;


/* Each vpage is 4 KB in size; i.e., 4 * 1024 bytes = 4096 bytes = NBPG, where
 * NBPG is # of bytes per page = 406.
 * So, vaddr can be computed using the following formula:
 *          vaddr = (vpage * NBPG)
 *          vpage = (vaddr / NBPG)
 */

#define VM_START_PAGE   4096
#define VM_IS_VPAGE_VALID(vpage)    (vpage >= VM_START_PAGE)
#define VADDR_IS_VALID(vaddr)       (((unsigned) vaddr / NBPG) >= VM_START_PAGE)
#define VADDR_TO_VPAGE(vaddr)       ((unsigned) vaddr / NBPG)
#define VPAGE_TO_VADDR(vpage)       (vpage * NBPG)
#define IS_VALID_PA(paddr)          (((unsigned) paddr >= 0) &&     \
                                        ((unsigned) paddr < (NBPG * NBPG)))

/* You can only ask no less than 1 byte and no more than what the vheap can 
 * support which is (4096 * 128) bytes.
 */
#define VH_IS_NBYTES_VALID(n)       ((n > 0) && (n < (NBPG * BS_MAX_NPAGES)))

/* Externs */
extern WORD* vgetmem(unsigned nbytes);
//extern void vfreemem(void *block, unsigned nbytes);
extern uint8_t vh_is_nbytes_valid(int pid, int nbytes);
extern void vh_release_vhlist(int pid);
/*------------------------------------- VM END ----------------------------------*/


/*------------------------------- PRINT UTILS START -----------------------------*/
/* Print utils */
#ifdef DBG_ON
extern void print_bsm_tab(uint8_t detail_flag);
extern void print_nonfree_bsm_tab(uint8_t detail_flag);
extern void print_bs_details(int bs_id, uint8_t detail_flag);
extern void print_frm_data(void);
extern void print_frm_id(int fr_id);
extern void print_pdbr(int pid);
extern void print_proc_vhlist(int pid);
extern void print_sc_tab(void);
extern void print_nru_tabs(void);
extern void print_nru_tab(int *nru_tab);
#endif /* DBG_ON */
/*-------------------------------- PRINT UTILS END ------------------------------*/

#endif /* _PAGING_H_ */
