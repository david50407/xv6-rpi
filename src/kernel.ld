OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)

ENTRY_SVC_STACK_SIZE = 0x1000;

SECTIONS
{
  /* the entry point, before enabling paging. The code to enable paing
   needs to have the same virtual/physical address. entry.S and start.c
   run in this initial setting.*/
  . = 0x10000;
  .start_sec : {
    build/entry.o(.text)
    build/start.o(.text .text.*)

    build/entry.o(.rodata .rodata.*)
    build/start.o(.rodata .rodata.*)

    build/entry.o(.data .data.*)
    build/start.o(.data .data.*)

    PROVIDE(edata_entry = .);

    build/entry.o(.bss .bss.* COMMON)
    build/start.o(.bss .bss.* COMMON)

    /*define a stack for the entry*/
    . = ALIGN(0x1000);
    . += ENTRY_SVC_STACK_SIZE;

    PROVIDE (svc_stktop = .);

    /* define the kernel page table, must be 16K and 16K-aligned*/
    . = ALIGN(0x4000);
    PROVIDE (_kernel_pgtbl = .);
    . += 0x4000;

    /* we also need a user page table*/
    PROVIDE (_user_pgtbl = .);
    . += 0x1000;

    PROVIDE(end_entry = .);
  }

  /*the kernel executes at the higher 2GB address space, but loaded
   at the lower memory (0x20000)*/
  . = 0x80020000;

  .text : AT(0x20000){
    *(.text .text.* .gnu.linkonce.t.*)
  }

  PROVIDE(etext = .);	/* Define the 'etext' symbol to this value */

  .rodata : {
    *(.rodata .rodata.* .gnu.linkonce.r.*)
  }

  /* aligned the data to a (4K) page, so it can be assigned
   different protection than the code*/
  . = ALIGN(0x1000);

  PROVIDE (data_start = .);

  .data : {
    *(.data .data.*)
  }

  PROVIDE (edata = .);

  .bss : {
    *(.bss .bss.* COMMON)
  }

  . = ALIGN(0x1000);
  PROVIDE (end = .);
}
