# Cross-compiling (e.g., on Mac OS X, install arm-none-eabi-gcc with MacPorts)
TOOLPREFIX = arm-none-eabi-
CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)as
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump

CFLAGS = -march=armv6 -fno-pic -static -fno-builtin -fno-strict-aliasing -Wall -Werror
LDFLAGS = -L.
ASFLAGS = -march=armv6 

