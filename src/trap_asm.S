//low-level trap handler glue code
#include "arm.h"

.text
.code 32

.global trap_swi
.global trap_irq
.global trap_reset
.global trap_und
.global trap_iabort
.global trap_dabort
.global trap_na
.global trap_fiq
.global trapret


# handle SWI, we allow nested SWI
trap_swi:
    # build trapframe on the stack
    STMFD   sp!, {r0-r12, r14}  // save context
    MRS     r2, spsr            // copy spsr to r2
    STMFD   r13!, {r2}          // save r2(spsr) to the stack
    STMFD   r13!, {r14}         // save r14 again to have one uniform trapframe
    STMFD   r13, {sp, lr}^      // save user mode sp and lr
    SUB     r13, r13, #8

    # call traps (trapframe *fp)
    MOV     r0, r13             // copy r13_svc to r0
    BL      swi_handler         // branch to the isr_swi

    # restore states
trapret:
    LDMFD   r13, {sp, lr}^      // restore user mode sp and lr
    ADD     r13, r13, #8
    LDMFD   r13!, {r14}         // restore r14
    LDMFD   r13!, {r2}          // restore spsr
    MSR     spsr_cxsf, r2
    LDMFD   r13!,{r0-r12, pc}^  // restore context and return


# handle IRQ, we allow nested IRQs
trap_irq:
    # save a few registers to the irq stack to provide scratch regs. 
    # r14 (lr_irq) contains the instruction (pc) to return to, need to
    # save it on the stack as r14 is banked
    SUB     r14, r14, #4            // r14 (lr) contains the interrupted PC
    STMFD   r13!, {r0-r2, r14}      //
    MRS     r1, spsr                // save spsr_irq
    MOV     r0, r13                 // save stack stop (r13_irq)
    ADD     r13, r13, #16           // reset the IRQ stack

    # switch to the SVC mode
    MRS     r2, cpsr
    BIC     r2, r2, #MODE_MASK
    ORR     r2, r2, #SVC_MODE
    MSR     cpsr_cxsf, r2

    # now, in SVC mode, sp, lr, pc (r13, r14, r15) are all banked 
    # build the trap frame
    LDR     r2, [r0, #12]           // read the r14_irq, then save it
    STMFD   r13!, {r2}
    STMFD   r13!, {r3-r12}          // r4-r12 are preserved (non-banked)
    LDMFD   r0, {r3-r5}             // copy r0-r2 over from irq stack
    STMFD   r13!, {r3-r5}
    STMFD   r13!, {r1}              // save spsr
    STMFD   r13!, {lr}              // save r14_svc

    STMFD   r13, {sp, lr}^          // save user mode sp and lr
    SUB     r13, r13, #8

    # get the parameters, then call the handler
    MOV     r0, r13                 // points to
    BL      irq_handler

    # restore the previous status
    B   trapret

# handle reset/undefine instruction/abort/not-assigned/fiq
# these handler does not allow nested handling
trap_reset:
    MOV     r14, #0                 // lr: not defined on reset
    STMFD   r13!, {r0-r12, r14}
    MRS     r2, spsr            // copy spsr to r2
    STMFD   r13!, {r2}          // save r2(spsr) to the stack
    STMFD   r13!, {r14}         // save r14 again (it is not really correct)
    STMFD   r13, {sp, lr}^      // save user mode sp and lr
    SUB     r13, r13, #8

    # call traps (trapframe *fp)
    MOV     r0, r13             // copy r13_svc to r0
    BL      reset_handler
    B       .

trap_und:
    STMFD   r13!, {r0-r12, r14} // lr: instruction after the undefined
    MRS     r2, spsr                // copy spsr to r2
    STMFD   r13!, {r2}              // save r2(spsr) to the stack
    STMFD   r13!, {r14}             // save r14 again (it is not really correct)
    STMFD   r13, {sp, lr}^          // save user mode sp and lr
    SUB     r13, r13, #8

    # call traps (trapframe *fp)
    MOV     r0, r13                 // save trapframe as the first parameter
    BL      und_handler
    B       .

trap_iabort:
    SUB     r14, r14, #4            // lr: instruction causing the abort
    STMFD   r13!, {r0-r12, r14}
    MRS     r2, spsr                // copy spsr to r2
    STMFD   r13!, {r2}              // save r2(spsr) to the stack
    STMFD   r13!, {r14}             // save r14 again (it is not really correct)
    STMFD   r13, {sp, lr}^          // save user mode sp and lr
    SUB     r13, r13, #8

    # call traps (trapframe *fp)
    MOV     r0, r13                 // save trapframe as the first parameter
    BL      iabort_handler
    B       .

trap_dabort:
    SUB     r14, r14, #8            // lr: instruction causing the abort
    STMFD   r13!, {r0-r12, r14}
    MRS     r2, spsr                // copy spsr to r2
    STMFD   r13!, {r2}              // save r2(spsr) to the stack
    STMFD   r13!, {r14}             // save r14 again (it is not really correct)
    STMFD   r13, {sp, lr}^          // save user mode sp and lr
    SUB     r13, r13, #8

    # call traps (trapframe *fp)
    MOV     r0, r13                 // save trapframe as the first parameter
    BL      dabort_handler
    MOV     r0, #2                  // #SYS_exit
    SWI     0x00
    B       exit
    #B       .                       // trapret

trap_na:
    STMFD   r13!, {r0-r12, r14} // should never happen, hardware error
    MRS     r2, spsr                // copy spsr to r2
    STMFD   r13!, {r2}              // save r2(spsr) to the stack
    STMFD   r13!, {r14}             // save r14 again (it is not really correct)
    STMFD   r13, {sp, lr}^          // save user mode sp and lr
    SUB     r13, r13, #8

    # call traps (trapframe *fp)
    MOV     r0, r13                 // save trapframe as the first parameter
    BL      na_handler
    B       .

trap_fiq:
    SUB     r14, r14, #4            // lr: return address after the fiq handler
    STMFD   r13!, {r0-r12, r14}
    MRS     r2, spsr                // copy spsr to r2
    STMFD   r13!, {r2}              // save r2(spsr) to the stack
    STMFD   r13!, {r14}             // save r14 again (it is not really correct)
    STMFD   r13, {sp, lr}^          // save user mode sp and lr
    SUB     r13, r13, #8

    # call traps (trapframe *fp)
    MOV     r0, r13                 // save trapframe as the first parameter
    BL      fiq_handler
    B       .

