/* adhanas */
/* control_reg.c - read_cr0 read_cr2 read_cr3 read_cr4
		   write_cr0 write_cr3 write_cr4 enable_pagine */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <paging.h>

unsigned long tmp;


/*-------------------------------------------------------------------------
 * read_cr0 - read CR0
 *-------------------------------------------------------------------------
 */
unsigned long read_cr0(void) {

  STATWORD ps;
  unsigned long local_tmp;

  disable(ps);

  asm("pushl %eax");
  asm("movl %cr0, %eax");
  asm("movl %eax, tmp");
  asm("popl %eax");

  local_tmp = tmp;

  restore(ps);

  return local_tmp;
}

/*-------------------------------------------------------------------------
 * read_cr2 - read CR2
 *-------------------------------------------------------------------------
 */

unsigned long read_cr2(void) {

  STATWORD ps;
  unsigned long local_tmp;

  disable(ps);

  asm("pushl %eax");
  asm("movl %cr2, %eax");
  asm("movl %eax, tmp");
  asm("popl %eax");

  local_tmp = tmp;

  restore(ps);

  return local_tmp;
}


/*-------------------------------------------------------------------------
 * read_cr3 - read CR3
 *-------------------------------------------------------------------------
 */

unsigned long read_cr3(void) {

  STATWORD ps;
  unsigned long local_tmp;

  disable(ps);

  asm("pushl %eax");
  asm("movl %cr3, %eax");
  asm("movl %eax, tmp");
  asm("popl %eax");

  local_tmp = tmp;

  restore(ps);

  return local_tmp;
}


/*-------------------------------------------------------------------------
 * read_cr4 - read CR4
 *-------------------------------------------------------------------------
 */

unsigned long read_cr4(void) {

  STATWORD ps;
  unsigned long local_tmp;

  disable(ps);

  asm("pushl %eax");
  asm("movl %cr4, %eax");
  asm("movl %eax, tmp");
  asm("popl %eax");

  local_tmp = tmp;

  restore(ps);

  return local_tmp;
}


/*-------------------------------------------------------------------------
 * write_cr0 - write CR0
 *-------------------------------------------------------------------------
 */

void write_cr0(unsigned long n) {

  STATWORD ps;

  disable(ps);

  tmp = n;
  asm("pushl %eax");
  asm("movl tmp, %eax");		/* mov (move) value at tmp into %eax register. 
					   "l" signifies long (see docs on gas assembler)	*/
  asm("movl %eax, %cr0");
  asm("popl %eax");

  restore(ps);

}


/*-------------------------------------------------------------------------
 * write_cr3 - write CR3
 *-------------------------------------------------------------------------
 */

void write_cr3(unsigned long n) {


  STATWORD ps;

  disable(ps);

  tmp = n;
  asm("pushl %eax");
  asm("movl tmp, %eax");                /* mov (move) value at tmp into %eax register.
                                           "l" signifies long (see docs on gas assembler)       */
  asm("movl %eax, %cr3");
  asm("popl %eax");

  restore(ps);

}


/*-------------------------------------------------------------------------
 * write_cr4 - write CR4
 *-------------------------------------------------------------------------
 */

void write_cr4(unsigned long n) {


  STATWORD ps;

  disable(ps);

  tmp = n;
  asm("pushl %eax");
  asm("movl tmp, %eax");                /* mov (move) value at tmp into %eax register.
                                           "l" signifies long (see docs on gas assembler)       */
  asm("movl %eax, %cr4");
  asm("popl %eax");

  restore(ps);

}


/*******************************************************************************
 * Name:    set_pdbr
 *
 * Desc:    Initializes the PDBR (CR3 register) with the given address. 
 *
 * Params: 
 *  pd_addr - a process's pdir address
 *
 * Returns: Nothing.
 ******************************************************************************/
void
set_pdbr(unsigned pd_addr)
{
    DTRACE_START;

    /* The CR3 register should contain the pd base address which will initiate 
     * the page lookup process for paging.
     */
    write_cr3(pd_addr);
    DTRACE("DBG$ %d %s> CR3 register loaded with pgd addr 0x%08x of "   \
            "pid %d\n",currpid, __func__, pd_addr, currpid);

    DTRACE_END;
    return;
}


/*******************************************************************************
 * Name:    enable_paging 
 *
 * Desc:    Enabls paging in the hardware by setting the PG bit in CR0 register.
 *
 * Params:  None. 
 *
 * Returns: Nothing.
 ******************************************************************************/
void 
enable_paging(void)
{
  
  unsigned long temp =  read_cr0();
  DTRACE_START;

  /* Set the PG bit (31st bit) in CR0 register to enable paging. Make sure
   * that PGDR (CR3) register is loaded with the appropriate page directory
   * to initiate page lookup process.
   */
  temp = temp | ( 0x1 << 31 ) | 0x1;
  write_cr0(temp); 
  DTRACE_END;
}

