// initialize section
#include "types.h"
#include "param.h"
#include "arm.h"
#include "mmu.h"
#include "defs.h"
#include "memlayout.h"

void _uart_putc(int c)
{
    volatile uint8 * uart0 = (uint8*)UART0;
    *uart0 = c;
}


void _puts (char *s)
{
    while (*s != '\0') {
        _uart_putc(*s);
        s++;
    }
}

void _putint (char *prefix, uint val, char* suffix)
{
    char* arr = "0123456789ABCDEF";
    int idx;

    if (prefix) {
        _puts(prefix);
    }

    for (idx = sizeof(val) * 8 - 4; idx >= 0; idx -= 4) {
        _uart_putc(arr[(val >> idx) & 0x0F]);
    }

    if (suffix) {
        _puts(suffix);
    }
}


// kernel page table, reserved in the kernel.ld
extern uint32 _kernel_pgtbl;
extern uint32 _user_pgtbl;

uint32 *kernel_pgtbl = &_kernel_pgtbl;
uint32 *user_pgtbl = &_user_pgtbl;

#define PDE_SHIFT 20

uint32 get_pde (uint32 virt)
{
    virt >>= PDE_SHIFT;
    return kernel_pgtbl[virt];
}

// setup the boot page table: dev_mem whether it is device memory
void set_bootpgtbl (uint32 virt, uint32 phy, uint len, int dev_mem )
{
    uint32	pde;
    int		idx;

    // convert all the parameters to indexes
    virt >>= PDE_SHIFT;
    phy  >>= PDE_SHIFT;
    len  >>= PDE_SHIFT;

    for (idx = 0; idx < len; idx++) {
        pde = (phy << PDE_SHIFT);

        if (!dev_mem) {
            // normal memory, make it kernel-only, cachable, bufferable
            pde |= (AP_KO << 10) | PE_CACHE | PE_BUF | KPDE_TYPE;
        } else {
            // device memory, make it non-cachable and non-bufferable
            pde |= (AP_KO << 10) | KPDE_TYPE;
        }

        // use different page table for user/kernel space
        if (virt < NUM_UPDE) {
            user_pgtbl[virt] = pde;
        } else {
            kernel_pgtbl[virt] = pde;
        }

        virt++;
        phy++;
    }
}

static void _flush_all (void)
{
    uint val = 0;

    // flush all TLB
    asm("MCR p15, 0, %[r], c8, c7, 0" : :[r]"r" (val):);

    // invalid entire data and instruction cache
    // asm ("MCR p15,0,%[r],c7,c5,0": :[r]"r" (val):);
    // asm ("MCR p15,0,%[r],c7,c6,0": :[r]"r" (val):);
}

void load_pgtlb (uint32* kern_pgtbl, uint32* user_pgtbl)
{
    uint	ret;
    char	arch;
    uint	val;

    // read the main id register to make sure we are running on ARMv6
    asm("MRC p15, 0, %[r], c0, c0, 0": [r]"=r" (ret)::);

    if (ret >> 24 == 0x41) {
        //_puts ("ARM-based CPU\n");
    }

    arch = (ret >> 16) & 0x0F;

    if ((arch != 7) && (arch != 0xF)) {
        _puts ("need AARM v6 or higher\n");
    }

    // we need to check the cache/tlb etc., but let's skip it for now

    // set domain access control: all domain will be checked for permission
    val = 0x55555555;
    asm("MCR p15, 0, %[v], c3, c0, 0": :[v]"r" (val):);

    // set the page table base registers. We use two page tables: TTBR0
    // for user space and TTBR1 for kernel space
    val = 32 - UADDR_BITS;
    asm("MCR p15, 0, %[v], c2, c0, 2": :[v]"r" (val):);

    // set the kernel page table
    val = (uint)kernel_pgtbl | 0x00;
    asm("MCR p15, 0, %[v], c2, c0, 1": :[v]"r" (val):);

    // set the user page table
    val = (uint)user_pgtbl | 0x00;
    asm("MCR p15, 0, %[v], c2, c0, 0": :[v]"r" (val):);

    // ok, enable paging using read/modify/write
    asm("MRC p15, 0, %[r], c1, c0, 0": [r]"=r" (val)::);

    val |= 0x80300D; // enable MMU, cache, write buffer, high vector tbl,
                     // disable subpage
    asm("MCR p15, 0, %[r], c1, c0, 0": :[r]"r" (val):);

    _flush_all();
}

extern void * edata_entry;
extern void * svc_stktop;
extern void kmain (void);
extern void jump_stack (void);

extern void * edata;
extern void * end;

// clear the BSS section for the main kernel, see kernel.ld
void clear_bss (void)
{
    memset(&edata, 0x00, (uint)&end-(uint)&edata);
}

void start (void)
{
	uint32  vectbl;
    _puts("starting xv6 for ARM...\n");

    // double map the low memory, required to enable paging
    // we do not map all the physical memory
    set_bootpgtbl(0, 0, INIT_KERNMAP, 0);
    set_bootpgtbl(KERNBASE, 0, INIT_KERNMAP, 0);

    // vector table is in the middle of first 1MB (0xF000)
    vectbl = P2V_WO (VEC_TBL & PDE_MASK);

    if (vectbl <= (uint)&end) {
        cprintf ("error: vector table overlaps kernel\n");
    }

    set_bootpgtbl(VEC_TBL, 0, 1 << PDE_SHIFT, 0);
    set_bootpgtbl(KERNBASE+DEVBASE, DEVBASE, DEV_MEM_SZ, 1);

    load_pgtlb (kernel_pgtbl, user_pgtbl);
    jump_stack ();
    
    // We can now call normal kernel functions at high memory
    clear_bss ();
    
    kmain ();
}
