#include "arm.h"
#include "memlayout.h"

.text
.code 32

.global _start

_start:
    # clear the entry bss section, the svc stack, and kernel page table
    LDR     r1, =edata_entry
    LDR     r2, =end_entry
    MOV     r3, #0x00

1:
    CMP     r1, r2
    STMLTIA r1!, {r3}
    BLT     1b

    # initialize stack pointers for svc modes
    MSR     CPSR_cxsf, #(SVC_MODE|NO_INT)
    LDR     sp, =svc_stktop

    BL      start
    B .

# during startup, kernel stack uses user address, now switch it to kernel addr
.global jump_stack
jump_stack:
    MOV     r0, sp
    ADD     r0, r0, #KERNBASE
    MOV     sp, r0
    MOV     pc, lr