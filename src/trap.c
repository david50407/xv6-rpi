// The ARM UART, a memory mapped device
#include "types.h"
#include "defs.h"
#include "param.h"
#include "arm.h"
#include "proc.h"

// trap routine
void swi_handler (struct trapframe *r)
{
    if (proc->killed)
        exit();
    proc->tf = r;
    syscall ();
    if (proc->killed)
        exit();
}

// trap routine
void irq_handler (struct trapframe *r)
{
    // proc points to the current process. If the kernel is
    // running scheduler, proc is NULL.
    if (proc != NULL) {
        proc->tf = r;
    }

    pic_dispatch (r);
}

// trap routine
void reset_handler (struct trapframe *r)
{
    cli();
    cprintf ("reset at: 0x%x \n", r->pc);
}

// trap routine
void und_handler (struct trapframe *r)
{
    cli();
    cprintf ("und at: 0x%x \n", r->pc);
}

// trap routine
void dabort_handler (struct trapframe *r)
{
    uint dfs, fa;

    cli();

    // read data fault status register
    asm("MRC p15, 0, %[r], c5, c0, 0": [r]"=r" (dfs)::);

    // read the fault address register
    asm("MRC p15, 0, %[r], c6, c0, 0": [r]"=r" (fa)::);
    
    cprintf ("data abort: instruction 0x%x, fault addr 0x%x, reason 0x%x \n",
             r->pc, fa, dfs);
    
    dump_trapframe (r);
}

// trap routine
void iabort_handler (struct trapframe *r)
{
    uint ifs;
    
    // read fault status register
    asm("MRC p15, 0, %[r], c5, c0, 0": [r]"=r" (ifs)::);

    cli();
    cprintf ("prefetch abort at: 0x%x (reason: 0x%x)\n", r->pc, ifs);
    dump_trapframe (r);
}

// trap routine
void na_handler (struct trapframe *r)
{
    cli();
    cprintf ("n/a at: 0x%x \n", r->pc);
}

// trap routine
void fiq_handler (struct trapframe *r)
{
    cli();
    cprintf ("fiq at: 0x%x \n", r->pc);
}

// low-level init code: in real hardware, lower memory is usually mapped
// to flash during startup, we need to remap it to SDRAM
void trap_init ( )
{
    volatile uint32 *ram_start;
    char *stk;
    int i;
    uint modes[] = {FIQ_MODE, IRQ_MODE, ABT_MODE, UND_MODE};

    // the opcode of PC relative load (to PC) instruction LDR pc, [pc,...]
    static uint32 const LDR_PCPC = 0xE59FF000U;

    // create the excpetion vectors
    ram_start = (uint32*)VEC_TBL;

    ram_start[0] = LDR_PCPC | 0x18; // Reset (SVC)
    ram_start[1] = LDR_PCPC | 0x18; // Undefine Instruction (UND)
    ram_start[2] = LDR_PCPC | 0x18; // Software interrupt (SVC)
    ram_start[3] = LDR_PCPC | 0x18; // Prefetch abort (ABT)
    ram_start[4] = LDR_PCPC | 0x18; // Data abort (ABT)
    ram_start[5] = LDR_PCPC | 0x18; // Not assigned (-)
    ram_start[6] = LDR_PCPC | 0x18; // IRQ (IRQ)
    ram_start[7] = LDR_PCPC | 0x18; // FIQ (FIQ)

    ram_start[8]  = (uint32)trap_reset;
    ram_start[9]  = (uint32)trap_und;
    ram_start[10] = (uint32)trap_swi;
    ram_start[11] = (uint32)trap_iabort;
    ram_start[12] = (uint32)trap_dabort;
    ram_start[13] = (uint32)trap_na;
    ram_start[14] = (uint32)trap_irq;
    ram_start[15] = (uint32)trap_fiq;

    // initialize the stacks for different mode
    for (i = 0; i < sizeof(modes)/sizeof(uint); i++) {
        stk = alloc_page ();

        if (stk == NULL) {
            panic("failed to alloc memory for irq stack");
        }

        set_stk (modes[i], (uint)stk);
    }
}

void dump_trapframe (struct trapframe *tf)
{
    cprintf ("r14_svc: 0x%x\n", tf->r14_svc);
    cprintf ("   spsr: 0x%x\n", tf->spsr);
    cprintf ("     r0: 0x%x\n", tf->r0);
    cprintf ("     r1: 0x%x\n", tf->r1);
    cprintf ("     r2: 0x%x\n", tf->r2);
    cprintf ("     r3: 0x%x\n", tf->r3);
    cprintf ("     r4: 0x%x\n", tf->r4);
    cprintf ("     r5: 0x%x\n", tf->r5);
    cprintf ("     r6: 0x%x\n", tf->r6);
    cprintf ("     r7: 0x%x\n", tf->r7);
    cprintf ("     r8: 0x%x\n", tf->r8);
    cprintf ("     r9: 0x%x\n", tf->r9);
    cprintf ("    r10: 0x%x\n", tf->r10);
    cprintf ("    r11: 0x%x\n", tf->r11);
    cprintf ("    r12: 0x%x\n", tf->r12);
    cprintf ("     pc: 0x%x\n", tf->pc);
}
