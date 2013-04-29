// Memory layout

// Key addresses for address space layout (see kmap in vm.c for layout)
#define EXTMEM	  		0x20000
#define KERNBASE  		0x80000000         // First kernel virtual address
#define KERNLINK  		(KERNBASE+EXTMEM)  // Address where kernel is linked

// we first map 1MB low memory containing kernel code.
#define INIT_KERNMAP 	0x100000

#ifndef __ASSEMBLER__

static inline uint v2p(void *a) { return ((uint) (a))  - KERNBASE; }
static inline void *p2v(uint a) { return (void *) ((a) + KERNBASE); }

#endif

#define V2P(a) (((uint) (a)) - KERNBASE)
#define P2V(a) (((void *) (a)) + KERNBASE)

#define V2P_WO(x) ((x) - KERNBASE)    // same as V2P, but without casts
#define P2V_WO(x) ((x) + KERNBASE)    // same as V2P, but without casts
