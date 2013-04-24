// BSP support routine
#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "proc.h"
#include "arm.h"
#include "mmu.h"

void cli (void)
{
    uint val;

    // ok, enable paging using read/modify/write
    asm("MRS %[v], cpsr": [v]"=r" (val)::);
    val |= DIS_INT;
    asm("MSR cpsr_cxsf, %[v]": :[v]"r" (val):);
}

void sti (void)
{
    uint val;

    // ok, enable paging using read/modify/write
    asm("MRS %[v], cpsr": [v]"=r" (val)::);
    val &= ~DIS_INT;
    asm("MSR cpsr_cxsf, %[v]": :[v]"r" (val):);
}

// return the cpsr used for user program
uint spsr_usr ()
{
    uint val;

    // ok, enable paging using read/modify/write
    asm("MRS %[v], cpsr": [v]"=r" (val)::);
    val &= ~MODE_MASK;
    val |= USR_MODE;

    return val;
}

// return whether interrupt is currently enabled
int int_enabled ()
{
    uint val;

    // ok, enable paging using read/modify/write
    asm("MRS %[v], cpsr": [v]"=r" (val)::);

    return !(val & DIS_INT);
}

// Pushcli/popcli are like cli/sti except that they are matched:
// it takes two popcli to undo two pushcli.  Also, if interrupts
// are off, then pushcli, popcli leaves them off.

void pushcli (void)
{
    int enabled;

    enabled = int_enabled();

    cli();

    if (cpu->ncli++ == 0) {
        cpu->intena = enabled;
    }
}

void popcli (void)
{
    if (int_enabled()) {
        panic("popcli - interruptible");
    }

    if (--cpu->ncli < 0) {
        cprintf("cpu (%d)->ncli: %d\n", cpu, cpu->ncli);
        panic("popcli -- ncli < 0");
    }

    if ((cpu->ncli == 0) && cpu->intena) {
        sti();
    }
}

// Record the current call stack in pcs[] by following the call chain.
// In ARM ABI, the function prologue is as:
//		push	{fp, lr}
//		add		fp, sp, #4
// so, fp points to lr, the return address
void getcallerpcs (void * v, uint pcs[])
{
    uint *fp;
    int i;

    fp = (uint*) v;

    for (i = 0; i < N_CALLSTK; i++) {
        if ((fp == 0) || (fp < (uint*) KERNBASE) || (fp == (uint*) 0xffffffff)) {
            break;
        }

        fp = fp - 1;			// points fp to the saved fp
        pcs[i] = fp[1];     // saved lr
        fp = (uint*) fp[0];	// saved fp
    }

    for (; i < N_CALLSTK; i++) {
        pcs[i] = 0;
    }
}

void show_callstk (char *s)
{
    int i;
    uint pcs[N_CALLSTK];

    cprintf("%s\n", s);

    getcallerpcs(get_fp(), pcs);

    for (i = N_CALLSTK - 1; i >= 0; i--) {
        cprintf("%d: 0x%x\n", i + 1, pcs[i]);
    }

}
