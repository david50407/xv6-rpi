#include "arm.h"

.text
.code 32

.global set_stk
.global get_fp

# set the stack for IRQ mode, we can use r0-r3 for free
# input: r0 - #mode, r1 - stk
set_stk:
    # switch to the IRQ mode
    MRS     r2, cpsr
    BIC     r2, r2, #MODE_MASK
    ORR     r2, r2, r0
    MSR     cpsr_cxsf, r2

    # set the stack pointer, sp register is banked, so we need to switch mode
    MOV     sp, r1

    # switch back to the SVC mode
    BIC     r2, r2, #MODE_MASK
    ORR     r2, r2, #SVC_MODE
    MSR     cpsr_cxsf, r2

    # return
    bx      lr

# return the frame pointer for the current function
get_fp:
    MOV     r0, fp
    bx      lr