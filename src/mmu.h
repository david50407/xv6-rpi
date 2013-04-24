// Definition for ARM MMU
#ifndef MMU_INCLUDE
#define MMU_INCLUDE

//
// Since ARMv6, you may use two page tables, one for kernel pages (TTBR1),
// and one for user pages (TTBR0). We use this architecture. Memory address
// lower than UVIR_BITS^2 is translated by TTBR0, while higher memory is
// translated by TTBR1.
// Kernel pages are create statically during system initialization. It use
// 1MB page mapping. User pages use 4K pages.
//


// access permission for page directory/page table entries.
#define AP_NA		0x00	// no access
#define AP_KO		0x01	// privilaged access, kernel: RW, user: no access
#define AP_KUR		0x02	// no write access from user, read allowed
#define AP_KU		0x03	// full access

// domain definition for page table entries
#define DM_NA		0x00	// any access causing a domain fault
#define DM_CLIENT	0x01	// any access checked against TLB (page table)
#define DM_RESRVED	0x02	// reserved
#define DM_MANAGER	0x03	// no access check

#define PE_CACHE	(1 << 3)// cachable
#define PE_BUF		(1 << 2)// bufferable

#define PE_TYPES	0x03	// mask for page type
#define KPDE_TYPE	0x02	// use "section" type for kernel page directory
#define UPDE_TYPE	0x01	// use "coarse page table" for user page directory
#define PTE_TYPE	0x02	// executable user page(subpage disable)

#define PDE_SHIFT	20		// shift how many bits to get PDE index
#define PDE_MASK	0xFFFFF	// offset for page directory entries
#define PD_SZ		(1 << PDE_SHIFT)
#define PD_UP(sz)	(((sz) + PD_SZ -1) & ~(PD_SZ -1))

#define KPD_IDX(v)	((uint)(v) >> PDE_SHIFT) // index for kernel page table entry
		// kernel page directory is a 16KB big table (4096 entries)

// the user space can have 256MB memory at maximum
#define UADDR_BITS  28	// the maximum user-application memory, 256MB
#define UADDR_SZ	(1 << UADDR_BITS)	// maximum user address space size

#define NPD_ENTRIES	(1 << (UADDR_BITS - PDE_SHIFT)) // how many entries in a PD
#define PD_IDX(v)	(KPD_IDX(v) & (NPD_ENTRIES - 1))

// each coarse page table is 1KB in size (256 entries) to map 1MB memory
#define	PTE_SHIFT	12					// shift how many bits to get PTE index
#define NPT_ENTRIES	(1 << (PDE_SHIFT - PTE_SHIFT))	// how many entries in a PT
#define PT_IDX(v)	(((uint)(v) >> PTE_SHIFT) & (NPT_ENTRIES -1))
#define PT_SZ		(NPT_ENTRIES << 2)			// user page table size (1K)
#define PT_ADDR(v)	((uint)(v) & ~(PT_SZ -1))	// physical address of the PTE

#define PG_SIZE		(1 << PTE_SHIFT)
#define PTE_ADDR(v) ((uint)(v) & ~(PG_SIZE -1))

#define PG_UP(sz)	(((sz)+PG_SIZE-1) & ~(PG_SIZE-1))
#define PG_DOWN(a)	(((a)) & ~(PG_SIZE-1))

#define PTE_AP(pte)	(((pte) >> 4) & 0x03)

#endif