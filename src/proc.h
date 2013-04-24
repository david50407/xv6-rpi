#ifndef PROC_INCLUDE_
#define PROC_INCLUDE_

// Per-CPU state, now we only support one CPU
struct cpu {
    uchar           id;             // index into cpus[] below
    struct context*   scheduler;    // swtch() here to enter scheduler
    volatile uint   started;        // Has the CPU started?

    int             ncli;           // Depth of pushcli nesting.
    int             intena;         // Were interrupts enabled before pushcli?

    // Cpu-local storage variables; see below
    struct cpu*     cpu;
    struct proc*    proc;           // The currently-running process.
};

extern struct cpu cpus[NCPU];
extern int ncpu;


extern struct cpu* cpu;
extern struct proc* proc;

//PAGEBREAK: 17
// Saved registers for kernel context switches. The context switcher
// needs to save the callee save register, as usually. For ARM, it is
// also necessary to save the banked sp (r13) and lr (r14) registers.
// There is, however, no need to save the user space pc (r15) because
// pc has been saved on the stack somewhere. We only include it here
// for debugging purpose. It will not be restored for the next process.
// According to ARM calling convension, r0-r3 is caller saved. We do
// not need to save sp_svc, as it will be saved in the pcb, neither
// pc_svc, as it will be always be the same value.
//
// Keep it in sync with swtch.S
//
struct context {
    // svc mode registers
    uint    r4;
    uint    r5;
    uint    r6;
    uint    r7;
    uint    r8;
    uint    r9;
    uint    r10;
    uint    r11;
    uint    r12;
    uint    lr;
};


enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state
struct proc {
    uint            sz;             // Size of process memory (bytes)
    pde_t*          pgdir;          // Page table
    char*           kstack;         // Bottom of kernel stack for this process
    enum procstate  state;          // Process state
    volatile int    pid;            // Process ID
    struct proc*    parent;         // Parent process
    struct trapframe*   tf;         // Trap frame for current syscall
    struct context* context;        // swtch() here to run process
    void*           chan;           // If non-zero, sleeping on chan
    int             killed;         // If non-zero, have been killed
    struct file*    ofile[NOFILE];  // Open files
    struct inode*   cwd;            // Current directory
    char            name[16];       // Process name (debugging)
};

// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
#endif
