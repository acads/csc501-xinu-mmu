/* adhanas */
/* pf_interrupt.S -- PA3, demand paging, low level page fault handler */

                .text
pf_error_code:  .long   0               
                .globl  pf_interrupt    /* high level interrupt handler     */
                .globl  pf_error_code   /* to store interrupt error code    */
pf_interrupt: 
    popl pf_error_code  /* pop the pf error code off the stack      */
    pushfl              /* save the eflag register                  */
    cli                 /* disable all interrupts                   */
    pushal              /* save all general purpose registers       */
    call pf_handler     /* call high level pf handler code          */
    popal               /* restore all general purpose registers    */
    popfl               /* restore eflag register                   */
    iret                /* return from interrupt handling           */
