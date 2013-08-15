xv6-rpi
=======

Project description
-------

[Xv6](http://pdos.csail.mit.edu/6.828/2012/xv6.html) is a simple Unix-like teaching operating system developed in 2006 by MIT for its operating system course. Xv6 is inspired by and modeled after the Sixth Edition Unix (aka V6). Xv6 is fairly complete educational operating system while remaining simply and manageable. It has most components in an early Unix system, such as processes, virtual memory, kernel-user separation, interrupt, file system...

The original xv6 is implemented for the x86 architecture. This is an effort to port xv6 to ARM, particularly [Raspberry Pi](http://www.raspberrypi.org/). The initial porting of xv6 to ARM (QEMU/ARMv6) has been completed by people in the department of [computer science](http://www.cs.fsu.edu/) in [Florida State University](http://www.fsu.edu/).

Debug
-------
1. use QEMU to dump a execution trace
`qemu-system-arm -M versatilepb -m 128 -cpu arm1176  -nographic -singlestep \
		-d exec,cpu,guest_errors -D qemu.log -kernel kernel.elf`

2. insert `show_callstk` in the kernel to dump current call stacks.

License
-------
The MIT License (MIT)

Copyright (c) 2013

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
