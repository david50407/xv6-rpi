#include "syscall.h"

#define SYSCALL(name) \
.globl name; \
name: \
	PUSH {r4};\
	MOV r4, r3;\
	MOV r3, r2;\
	MOV r2, r1;\
	MOV r1, r0;\
	MOV r0, #SYS_ ## name;\
	swi 0x00;\
	POP {r4};\
	bx lr;

SYSCALL(fork)
SYSCALL(exit)
SYSCALL(wait)
SYSCALL(pipe)
SYSCALL(read)
SYSCALL(write)
SYSCALL(close)
SYSCALL(kill)
SYSCALL(exec)
SYSCALL(open)
SYSCALL(mknod)
SYSCALL(unlink)
SYSCALL(fstat)
SYSCALL(link)
SYSCALL(mkdir)
SYSCALL(chdir)
SYSCALL(dup)
SYSCALL(getpid)
SYSCALL(sbrk)
SYSCALL(sleep)
SYSCALL(uptime)
