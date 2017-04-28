# Copyright (c) 1992 The Regents of the University of California.
# All rights reserved.  See copyright.h for copyright notice and limitation 
# of liability and disclaimer of warranty provisions.

MAKE = make
LPR = lpr

.PHONY:	all clean print

all: 
	$(MAKE) -C userprog depend 
	$(MAKE) -C userprog nachos
	$(MAKE) -C lib
	$(MAKE) -C test

# don't delete executables in "test" in case there is no cross-compiler
clean:
	-rm -f *~ */{core*,nachos,DISK,*.o,swtch.s,swap,swapfile,.depend,.gdb_history} test/{*.coff,*~} bin/{coff2flat,disassemble,out,*~} lib/lib.a
	cd test; make clean

print:
	$(LPR) Makefile* */Makefile
	$(LPR) threads/*.h threads/*.cc threads/*.s
	$(LPR) userprog/*.h userprog/*.cc
	$(LPR) filesys/*.h filesys/*.cc
	$(LPR) network/*.h network/*.cc
	$(LPR) machine/*.h machine/*.cc
	$(LPR) bin/coff.h
	$(LPR) test/*.h test/*.c test/*.s

TAGS:
	etags -l c++ --members bin/elf.h debug/*.{cc,h} filesys/*.{cc,h} \
	machine/*.{cc,h} network/*.{cc,h} threads/*.{cc,h} userprog/*.{cc,h}
