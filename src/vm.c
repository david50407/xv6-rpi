#include "param.h"
#include "types.h"
#include "defs.h"
#include "arm.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "elf.h"

extern char data[];  // defined by kernel.ld
pde_t *kpgdir;  // for use in scheduler()


// Xv6 can only allocate memory in 4KB blocks. This is fine
// for x86. ARM's page table and page directory (for 28-bit
// user address) have a size of 1KB. We use a simple allocation
// scheme to manage it. 
struct run {
    struct run *next;
};

struct {
    struct spinlock lock;
    struct run *freelist;
} kpt_mem;


void init_vmm (void)
{
	initlock (&kpt_mem.lock, "vm");
	kpt_mem.freelist = NULL;
}

static void kpt_free(char *v)
{
    struct run *r;

	acquire(&kpt_mem.lock);

	r = (struct run*)v;
    r->next = kpt_mem.freelist;
    kpt_mem.freelist = r;

	release(&kpt_mem.lock);
}

void* kpt_alloc(void)
{
    struct run	*r;
	char		*p;

	acquire(&kpt_mem.lock);
	
    r = kpt_mem.freelist;

	// no cache of page tables, allocate a new page (4KB)
	if (r == NULL) {
		p = kalloc();

		if (p == NULL) {
			panic ("oom: kpt_alloc");
		}

		// only use the first 1K, release others to the pool
		kpt_free (p + PT_SZ);
		kpt_free (p + PT_SZ * 2);
		kpt_free (p + PT_SZ * 3);

		r = (struct run*)p;
	} else {
        kpt_mem.freelist = r->next;
	}
	
	release(&kpt_mem.lock);

	memset(r, 0, PT_SZ);
    return (char*)r;
}


// Return the address of the PTE in page directory that corresponds to
// virtual address va.  If alloc!=0, create any required page table pages.
static pte_t* walkpgdir(pde_t *pgdir, const void *va, int alloc)
{
	pde_t *pde;
	pte_t *pgtab;

	// pgdir points to the page directory, get the page direcotry entry (pde)
	pde = &pgdir[PD_IDX(va)];
	
	if(*pde & PE_TYPES){
		pgtab = (pte_t*)p2v(PT_ADDR(*pde));
		
	} else {
		if(!alloc || (pgtab = (pte_t*)kpt_alloc()) == 0) {
			return 0;
		}

		// Make sure all those PTE_P bits are zero.
		memset(pgtab, 0, PT_SZ);
		
		// The permissions here are overly generous, but they can
		// be further restricted by the permissions in the page table
		// entries, if necessary.
		*pde = v2p(pgtab) | UPDE_TYPE;
	}
	
	return &pgtab[PT_IDX(va)];
}

// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size might not
// be page-aligned.
static int mappages(pde_t *pgdir, void *va, uint size, uint pa, int ap)
{
	char *a, *last;
	pte_t *pte;

	a    = (char*)PG_DOWN((uint)va);
	last = (char*)PG_DOWN(((uint)va) + size - 1);
	
	for(;;){
		if((pte = walkpgdir(pgdir, a, 1)) == 0) {
			return -1;
		}
		
		if(*pte & PE_TYPES) {
			panic("remap");
		}
		
		*pte = pa | ((ap & 0x3) << 4) | PE_CACHE | PE_BUF | PTE_TYPE ;

		if(a == last) {
			break;
		}

		a  += PG_SIZE;
		pa += PG_SIZE;
	}
	
	return 0;
}

// flush all TLB
static void flush_tlb (void)
{
	uint val = 0;
	asm("MCR p15, 0, %[r], c8, c7, 0" : :[r]"r" (val):);
}

// Switch to the user page table (TTBR0)
void switchuvm(struct proc *p)
{
	uint val;

	pushcli();

	if(p->pgdir == 0) {
		panic("switchuvm: no pgdir");
	}

	val = (uint)V2P(p->pgdir) | 0x00;
	
	asm("MCR p15, 0, %[v], c2, c0, 0": :[v]"r" (val):);
	flush_tlb ();

	popcli();
}

// Load the initcode into address 0 of pgdir. sz must be less than a page.
void inituvm(pde_t *pgdir, char *init, uint sz)
{
	char *mem;

	if(sz >= PG_SIZE) {
		panic("inituvm: more than a page");
	}
	
	mem = kalloc();
	memset(mem, 0, PG_SIZE);
	mappages(pgdir, 0, PG_SIZE, v2p(mem), AP_KU);
	memmove(mem, init, sz);
}

// Load a program segment into pgdir.  addr must be page-aligned
// and the pages from addr to addr+sz must already be mapped.
int loaduvm(pde_t *pgdir, char *addr, struct inode *ip, uint offset, uint sz)
{
	uint i, pa, n;
	pte_t *pte;

	if((uint) addr % PG_SIZE != 0) {
		panic("loaduvm: addr must be page aligned");
	}

	for(i = 0; i < sz; i += PG_SIZE){
		if((pte = walkpgdir(pgdir, addr+i, 0)) == 0) {
			panic("loaduvm: address should exist");
		}

		pa = PTE_ADDR(*pte);
		
		if(sz - i < PG_SIZE) {
			n = sz - i;
		} else {
			n = PG_SIZE;
		}
		
		if(readi(ip, p2v(pa), offset+i, n) != n) {
			return -1;
		}
	}
	
	return 0;
}

// Allocate page tables and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
int allocuvm(pde_t *pgdir, uint oldsz, uint newsz)
{
	char *mem;
	uint a;

	if(newsz >= UADDR_SZ) {
		return 0;
	}

	if(newsz < oldsz) {
		return oldsz;
	}

	a = PG_UP(oldsz);
	
	for(; a < newsz; a += PG_SIZE){
		mem = kalloc();

		if(mem == 0){
			cprintf("allocuvm out of memory\n");
			deallocuvm(pgdir, newsz, oldsz);
			return 0;
		}
		
		memset(mem, 0, PG_SIZE);
		mappages(pgdir, (char*)a, PG_SIZE, v2p(mem), AP_KU);
	}
	
	return newsz;
}

// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
int deallocuvm(pde_t *pgdir, uint oldsz, uint newsz)
{
	pte_t	*pte;
	uint	a;
	uint	pa;

	if(newsz >= oldsz) {
		return oldsz;
	}
	
	for(a = PG_UP(newsz); a  < oldsz; a += PG_SIZE){
		pte = walkpgdir(pgdir, (char*)a, 0);
		
		if(!pte) {
			// pte == 0 --> no page table for this entry
			// round it up to the next page directory
			a = PD_UP (a);

		} else if((*pte & PE_TYPES) != 0){
			pa = PTE_ADDR(*pte);

			if(pa == 0) {
				panic("deallocuvm");
			}

			kfree(p2v(pa));
			*pte = 0;
		}
	}
	
	return newsz;
}

// Free a page table and all the physical memory pages
// in the user part.
void freevm(pde_t *pgdir)
{
	uint i;
	char *v;
	
	if(pgdir == 0) {
		panic("freevm: no pgdir");
	}

	// release the user space memroy, but not page tables
	deallocuvm(pgdir, UADDR_SZ, 0);

	// release the page tables
	for(i = 0; i < NPD_ENTRIES; i++){
		if(pgdir[i] & PE_TYPES){
			v = p2v(PT_ADDR(pgdir[i]));
			kpt_free(v);
		}
	}
	
	kpt_free((char*)pgdir);
}

// Clear PTE_U on a page. Used to create an inaccessible page beneath
// the user stack (to trap stack underflow).
void clearpteu(pde_t *pgdir, char *uva)
{
	pte_t *pte;

	pte = walkpgdir(pgdir, uva, 0);
	if(pte == 0) {
		panic("clearpteu");
	}

	// in ARM, we change the AP field (ap & 0x3) << 4)
	*pte = (*pte & ~(0x03 << 4)) | AP_KO << 4;
}

// Given a parent process's page table, create a copy
// of it for a child.
pde_t* copyuvm(pde_t *pgdir, uint sz)
{
	pde_t	*d;
	pte_t	*pte;
	uint	pa, i, ap;
	char	*mem;

	// allocate a new first level page directory
	d = kpt_alloc ();
	if (d == NULL) {
		return NULL;
	}

	// copy the whole address space over (no COW)
	for(i = 0; i < sz; i += PG_SIZE){
		if((pte = walkpgdir(pgdir, (void *) i, 0)) == 0) {
			panic("copyuvm: pte should exist");
		}

		if(!(*pte & PE_TYPES)) {
			panic("copyuvm: page not present");
		}

		pa = PTE_ADDR (*pte);
		ap = PTE_AP   (*pte);
		
		if((mem = kalloc()) == 0) {
			goto bad;
		}
		
		memmove(mem, (char*)p2v(pa), PG_SIZE);
		
		if(mappages(d, (void*)i, PG_SIZE, v2p(mem), ap) < 0) {
			goto bad;
		}
	}
	return d;

bad:
	freevm(d);
	return 0;
}

//PAGEBREAK!
// Map user virtual address to kernel address.
char* uva2ka(pde_t *pgdir, char *uva)
{
	pte_t *pte;

	pte = walkpgdir(pgdir, uva, 0);

	// make sure it exists
	if((*pte & PE_TYPES) == 0) {
		return 0;
	}

	// make sure it is a user page
	if(PTE_AP(*pte) != AP_KU) {
		return 0;
	}
	
	return (char*)p2v(PTE_ADDR(*pte));
}

// Copy len bytes from p to user address va in page table pgdir.
// Most useful when pgdir is not the current page table.
// uva2ka ensures this only works for user pages.
int copyout(pde_t *pgdir, uint va, void *p, uint len)
{
	char *buf, *pa0;
	uint n, va0;

	buf = (char*)p;
	
	while(len > 0){
		va0 = (uint)PG_DOWN(va);
		pa0 = uva2ka(pgdir, (char*)va0);
		
		if(pa0 == 0) {
			return -1;
		}

		n = PG_SIZE - (va - va0);

		if(n > len) {
			n = len;
		}

		memmove(pa0 + (va - va0), buf, n);
		
		len -= n;
		buf += n;
		va = va0 + PG_SIZE;
	}
	
	return 0;
}
