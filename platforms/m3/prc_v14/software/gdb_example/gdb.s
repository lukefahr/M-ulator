/* 
 *
 * GDB Hooks for GDBoverMBUS support
 *
 *
 * Andrew Lukefahr
 * lukefahr@umich.edu
 *
 * Much of this is borrowed from:
 *  - https://github.com/adamheinrich/os.h/blob/master/src/os_pendsv_handler.s
 *  - The Definitive Guide to ARM Cortex-M0 and Cortex-M0+ Processors Section 10.7.1
 */

.syntax unified
.cpu cortex-m0
.fpu softvfp

.thumb 

.global handler_svcall
.type handler_svcall, %function
handler_svcall:


    /*
    * Exception frame saved by the NVIC hardware onto stack:
    * +------+
    * |      | <- SP before interrupt (orig. SP)
    * | xPSR |
    * |  PC  |
    * |  LR  |
    * |  R12 |
    * |  R3  |
    * |  R2  |
    * |  R1  |
    * |  R0  | <- SP after entering interrupt (orig. SP + 32 bytes)
    * +------+
    */

    /*
    * Registers saved by the software (PendSV_Handler):
    * +------+
    * |  R7  |
    * |  R6  |
    * |  R5  |
    * |  R4  |
    * |  R11 |
    * |  R10 |
    * |  R9  |
    * |  R8  | <- Saved SP (orig. SP + 64 bytes)
    * +------+
    */
    /* we can push r4-r7 directly, r8-r11 we have to move into a lower
     * register first, then store it */
    push    {r4,r5,r6,r7}
    mov     r4, r8
    mov     r5, r9
    mov     r6, r10
    mov     r7, r11
    push    {r4,r5,r6,r7}

    /*
    * Finally save the (recalculated) origional stack pointer and 
    * the Special LR is saved by the software (PendSV_Handler):
    * +------+
    * |  SP  |  this is the origional (pre-svc) sp value 
    * |ISR_LR| <- Saved SP (orig. SP + 72 bytes)
    * +------+
    */
    mov     r1, sp
    subs    r1, #68 //back to origional sp before svc
    push    {r1}
    push    {lr}

    /* Now we can call the C svc_handler 
     * adapted from 'the_definitive_guide_to_arm_cortex_m0_and_cortex_m0+_processors'
     */

    /* make the stack pointer the first argument to the C svc_handler
     * but first we need to figure out which stack we're using
     * and we can do that by testing the special LR
     */
    movs    r0, #4
    mov     r1, LR // SPECIAL LR
    tst     r0, r1
    beq     stacking_used_MSP

    stacking_used_PSP:
    mrs     r0, PSP         // first parameter - stacking was using PSP
    b       handler_svcall_2 

    stacking_used_MSP:
    mrs     r0, MSP         // first parameter - stacking was using MSP
  
    handler_svcall_2:
    /* ok, now we can actually call the c svc_handler */
    ldr     r2, =handler_svcall_main // branch to C handler
    blx     r2

    /* pop the Special LR */
    pop     {r1} 

    /* pop the dummy SP */
    pop     {r4} 

    /* pop r8-r11 */
    pop     {r4,r5,r6,r7}
    mov     r8, r4
    mov     r9, r5
    mov     r10, r6
    mov     r11, r7

    /* pop r4-r7 */
    pop     {r4,r5,r6,r7}

    /* and return from SVC */
    bx      r1


//    /* Disable interrupts: */
//    cpsid    i
//
//    /*
//    Registers saved by the software (PendSV_Handler):
//    +------+
//    |  R7  |
//    |  R6  |
//    |  R5  |
//    |  R4  |
//    |  R11 |
//    |  R10 |
//    |  R9  |
//    |  R8  | <- Saved SP (orig. SP + 64 bytes)
//    +------+
//    */
//
//    /* Save registers R4-R11 (32 bytes) onto current PSP (process stack
//    pointer) and make the PSP point to the last stacked register (R8):
//    - The MRS/MSR instruction is for loading/saving a special registers.
//    - The STMIA inscruction can only save low registers (R0-R7), it is
//    therefore necesary to copy registers R8-R11 into R4-R7 and call
//    STMIA twice. */
//    mrs    r0, psp
//    subs    r0, #16
//    stmia    r0!,{r4-r7}
//    mov    r4, r8
//    mov    r5, r9
//    mov    r6, r10
//    mov    r7, r11
//    subs    r0, #32
//    stmia    r0!,{r4-r7}
//    subs    r0, #16
//
//    /* Save current task's SP: */
//    ldr    r2, =os_curr_task
//    ldr    r1, [r2]
//    str    r0, [r1]
//
//    /* Load next task's SP: */
//    ldr    r2, =os_next_task
//    ldr    r1, [r2]
//    ldr    r0, [r1]
//
//    /* Load registers R4-R11 (32 bytes) from the new PSP and make the PSP
//    point to the end of the exception stack frame. The NVIC hardware
//    will restore remaining registers after returning from exception): */
//    ldmia    r0!,{r4-r7}
//    mov    r8, r4
//    mov    r9, r5
//    mov    r10, r6
//    mov    r11, r7
//    ldmia    r0!,{r4-r7}
//    msr    psp, r0
//
//    /* EXC_RETURN - Thread mode with PSP: */
//     ldr r0, =0xFFFFFFFD
//
//    /* Re-Enable interrupts: */
//    cpsie    i
//
//    bx    r0



