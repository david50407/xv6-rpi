# Initial process execs /init.

#include "syscall.h"

.globl start

# exec(init, argv)
start:
    LDR     r1, =init
    LDR     r2, =argv
    MOV     r0, #SYS_exec
    SWI     0x00

exit:
    MOV     r0, #SYS_exit
    SWI     0x00
    B       exit

# char init[] = "/init\0";
init:
    .string "/init\0"

# char *argv[] = { init, 0 };
.p2align 2
argv:
    .word init
    .word 0

