CC=gcc
CFLAGS=-w -c -Wall -pedantic

all: snp schat 

snp: snp.o
	$(CC) snp.o -o snp
snp.o: snp.c
	$(CC) $(CFLAGS) snp.c 
schat: schat.o
	$(CC) schat.o -o schat
schat.o: schat.c
	$(CC) $(CFLAGS) schat.c

clean:
	rm *.o
