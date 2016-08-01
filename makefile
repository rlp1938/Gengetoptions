CC=gcc
CFLAGS=-c -Wall -Wextra -g -O0 -D_GNU_SOURCE=1

all: x

x: fileops.o optgenopt.o stringops.o x.o
	$(CC) fileops.o optgenopt.o stringops.o x.o -o x

x.o: x.c fileops.h stringops.h
	$(CC) $(CFLAGS) x.c
	
fileops.o: fileops.c fileops.h stringops.h
	$(CC) $(CFLAGS) fileops.c

stringops.o: stringops.c stringops.h fileops.h
	$(CC) $(CFLAGS) stringops.c

optgenopt.o: optgenopt.c optgenopt.h stringops.h fileops.h
	$(CC) $(CFLAGS) optgenopt.c

clean:
	rm *.o x
