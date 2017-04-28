# use normal make for this Makefile
#
# Makefile for building user programs to run on top of Nachos
#
# Several things to be aware of:
#
#    Nachos assumes that the location of the program startup routine (the
# 	location the kernel jumps to when the program initially starts up)
#       is at location 0.  This means: start.o must be the first .o passed 
# 	to ld, in order for the routine "Start" to be loaded at location 0
#

PROGRAMS = halt shell matmult matmult2 matmult4 matmult8 sort exit-prog fork fork-yield count nice access1 access2
# if you are cross-compiling, you need to point to the right executables
# and change the flags to ld and the build procedure for as
GCCDIR = /usr/local/nachos/bin/
LDFLAGS = -T script
ASFLAGS = # -mips
CPPFLAGS = $(INCDIR)

# if you aren't cross-compiling:
# GCCDIR =
# LDFLAGS = -N -T 0
# ASFLAGS =
# CPPFLAGS = -P $(INCDIR)

CC = $(GCCDIR)mipsel-elf-gcc
AS = $(GCCDIR)mipsel-elf-as
LD = $(GCCDIR)mipsel-elf-ld

CPP = /lib/cpp
INCDIR =-I../userprog -I../threads -I../lib
CFLAGS = -fno-builtin -g -c $(INCDIR)
ASFLAGS = 

all: $(PROGRAMS)

clean: 
	rm -f $(PROGRAMS) *.o *~ results swapfile*

#SYSCALL_CHANGE

start.o: ../userprog/start.s 
	$(CPP) $(CPPFLAGS) ../userprog/start.s > strt.s
	$(AS) $(ASFLAGS) -o start.o strt.s
	rm strt.s

Systemcalls.o: ../userprog/Systemcalls.s
	$(CPP) $(CPPFLAGS) ../userprog/Systemcalls.s > Syscalls.s
	$(AS) $(ASFLAGS) -o Systemcalls.o Syscalls.s
	rm Syscalls.s

halt.o: halt.c
	$(CC) $(CFLAGS) -c halt.c
halt: halt.o start.o Systemcalls.o
	$(LD) $(LDFLAGS) start.o Systemcalls.o halt.o -o $@

shell.o: shell.c
	$(CC) $(CFLAGS) -c shell.c
shell: shell.o start.o Systemcalls.o  ../lib/lib.a
	$(LD) $(LDFLAGS) start.o Systemcalls.o shell.o -o $@ ../lib/lib.a

sort.o: sort.c
	$(CC) $(CFLAGS) -c sort.c
sort: sort.o start.o Systemcalls.o ../lib/lib.a
	$(LD) $(LDFLAGS) start.o Systemcalls.o sort.o -o $@ ../lib/lib.a

matmult.o: matmult.c
	$(CC) $(CFLAGS) -c matmult.c
matmult: matmult.o start.o Systemcalls.o ../lib/lib.a
	$(LD) $(LDFLAGS) start.o Systemcalls.o matmult.o -o $@ ../lib/lib.a

matmult2.o: matmult.c
	$(CC) $(CFLAGS) -DFORK2 -o matmult2.o -c matmult.c
matmult2: matmult2.o start.o Systemcalls.o ../lib/lib.a
	$(LD) $(LDFLAGS) start.o Systemcalls.o matmult2.o -o $@ ../lib/lib.a

matmult4.o: matmult.c
	$(CC) $(CFLAGS) -DFORK4 -o matmult4.o -c matmult.c
matmult4: matmult4.o start.o Systemcalls.o ../lib/lib.a
	$(LD) $(LDFLAGS) start.o Systemcalls.o  matmult4.o -o $@ ../lib/lib.a

matmult8.o: matmult.c
	$(CC) $(CFLAGS) -DFORK8 -o matmult8.o -c matmult.c
matmult8: matmult8.o start.o Systemcalls.o ../lib/lib.a
	$(LD) $(LDFLAGS) start.o Systemcalls.o matmult8.o -o $@ ../lib/lib.a

exit-prog.o: exit-prog.c
	$(CC) $(CFLAGS) -c exit-prog.c
exit-prog: exit-prog.o start.o Systemcalls.o
	$(LD) $(LDFLAGS) start.o Systemcalls.o exit-prog.o -o $@

fork.o: fork.c
	$(CC) $(CFLAGS) -c fork.c
fork: fork.o start.o Systemcalls.o
	$(LD) $(LDFLAGS) start.o Systemcalls.o fork.o -o $@

fork-yield.o: fork-yield.c
	$(CC) $(CFLAGS) -c fork-yield.c
fork-yield: fork-yield.o start.o Systemcalls.o
	$(LD) $(LDFLAGS) start.o Systemcalls.o fork-yield.o -o $@

count.o: count.c
	$(CC) $(CFLAGS) -c count.c
count: count.o start.o Systemcalls.o
	$(LD) $(LDFLAGS) start.o Systemcalls.o count.o -o $@

nice.o: nice.c
	$(CC) $(CFLAGS) -c nice.c
nice: nice.o start.o Systemcalls.o
	$(LD) $(LDFLAGS) start.o Systemcalls.o nice.o -o $@

access1.o: access1.c phys.h
	$(CC) $(CFLAGS) -c access1.c
access1: access1.o start.o Systemcalls.o
	$(LD) $(LDFLAGS) start.o Systemcalls.o access1.o -o $@

access2.o: access2.c phys.h
	$(CC) $(CFLAGS) -c access2.c
access2: access2.o start.o Systemcalls.o
	$(LD) $(LDFLAGS) start.o Systemcalls.o access2.o -o $@
