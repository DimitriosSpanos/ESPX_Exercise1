CC=gcc
CFLAGS=-O3

default: all

prod-cons:
	$(CC) $(CFLAGS) -o prod-cons prod-cons.c -lpthread

.PHONY: clean

all: prod-cons

test:
	@printf "\n** Testing Producer-Consumer\n\n"
	./prod-cons

clean:
	rm -f prod-cons 
