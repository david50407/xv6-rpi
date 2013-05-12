# specify path to QEMU, installed with MacPorts 
QEMU = qemu-system-arm

include makefile.inc

# link the libgcc.a for __aeabi_idiv. ARM has no native support for div
LIBS=-lgcc

OBJS = \
	arm.o\
	asm.o\
	bio.o\
	buddy.o\
	console.o\
	exec.o\
	file.o\
	fs.o\
	log.o\
	main.o\
	memide.o\
	picirq.o\
	pipe.o\
	proc.o\
	spinlock.o\
	start.o\
	string.o\
	swtch.o\
	syscall.o\
	sysfile.o\
	sysproc.o\
	timer.o\
	trap_asm.o\
	trap.o\
	uart.o\
	vm.o
	
kernel.elf: $(OBJS) entry.o kernel.ld initcode fs.img
	$(LD) $(LDFLAGS) -T kernel.ld -o kernel.elf entry.o $(OBJS) $(LIBS) -b binary initcode fs.img
	$(OBJDUMP) -S kernel.elf > kernel.asm
	$(OBJDUMP) -t kernel.elf | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > kernel.sym

qemu: kernel.elf
	$(QEMU) -M versatilepb -m 128 -cpu arm1176  -nographic -kernel kernel.elf

#initcode is linked into the kernel, it will be used to craft the first process
initcode: initcode.S
	$(CC) $(CFLAGS) -nostdinc -I. -c initcode.S
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o initcode.out initcode.o
	$(OBJCOPY) -S -O binary initcode.out initcode
	$(OBJDUMP) -S initcode.o > initcode.asm

fs.img: 
	make -C usr
	cp usr/fs.img fs.img

clean: 
	rm -f *.o *.d *.asm *.sym vectors.S bootblock entryother \
	initcode initcode.out kernel xv6.img fs.img kernel.elf memfs mkfs
	make -C usr clean
