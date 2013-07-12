# Cross-compiling (e.g., on Mac OS X, install arm-none-eabi-gcc with MacPorts)
CROSSCOMPILE := arm-none-eabi-

CC = $(CROSSCOMPILE)gcc
AS = $(CROSSCOMPILE)as
LD = $(CROSSCOMPILE)ld
OBJCOPY = $(CROSSCOMPILE)objcopy
OBJDUMP = $(CROSSCOMPILE)objdump

CFLAGS = -march=armv6 -fno-pic -static -fno-builtin -fno-strict-aliasing -Wall -Werror -I.
LDFLAGS = -L.
ASFLAGS = -march=armv6 

LIBGCC = $(shell $(CC) -print-libgcc-file-name)
