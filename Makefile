CC ?= clang
CFLAGS ?= -Wall -pedantic -std=c99

rpk_extract.exe: main.o rpack.o
	$(CC) $(OPTS) main.o rpack.o -o rpk_extract.exe

main.o: main.c rpack.h
	$(CC) $(OPTS)  -c main.c

rpack.o: rpack.c rpack.h
	$(CC) $(OPTS) -c rpack.c

clean:
	rm -f *.o *.exe
	rm -rf test_extract