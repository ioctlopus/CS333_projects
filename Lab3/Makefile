#Ben Rimedio - CS333 - Fa22 - Lab 3 - Makefile

CC = gcc
DEBUG = -g
DEFINES =

CFLAGS = $(DEBUG) -Wall -Werror -Wshadow -Wunreachable-code -Wredundant-decls -Wold-style-definition -Wmissing-prototypes -Wdeclaration-after-statement -Wno-return-local-addr -Wuninitialized -Wmissing-declarations -Wunsafe-loop-optimizations $(DEFINES)

all: primesMT

primesMT: primesMT.o
	$(CC) $(CFLAGS) -o primesMT primesMT.o -lm -pthread
	chmod a+rx,g-w primesMT

primesMT.o: main.c
	$(CC) $(CFLAGS) -c -o primesMT.o main.c -lm -pthread

clean:
	rm -f $(PROGS) *.o *~ \#*

tar:
	tar -czvf brimedio-primesMT.tar.gz ./*.[ch] ./Makefile

scpt:
	scp ./* brimedio@babbage.cs.pdx.edu:/u/brimedio/cs333/Lab3

scpf:
	scp brimedio@babbage.cs.pdx.edu:/u/brimedio/cs333/Lab3/* ./
