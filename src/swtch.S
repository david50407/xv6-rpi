# Context switch
#
#   void swtch(struct context **old, struct context *new);
#
# Save current register context in old
# and then load register context from new.
# The stack is as r4_svc-r12_svc, lr_svc, sp_usr, lr_usr, and pc_usr
.global swtch

swtch:
    STMFD   r13!, {r4-r12, lr}  // push svc r4-r12, lr to the stack

    # switch the stack
    STR     r13, [r0]           // save current sp to the old PCB (**old)
    MOV     r13, r1             // load the next stack

    # load the new registers. pc_usr is not restored here because
    # LDMFD^ will switch mode if pc_usr is loaded. We just simply
    # pop it out as pc_usr is saved on the stack, and will be loaded
    # when we return from kernel to user space (swi or interrupt return)

    LDMFD   r13!, {r4-r12, lr}  // pop svc r4-r12, lr

    # return to the caller
    bx      lr

