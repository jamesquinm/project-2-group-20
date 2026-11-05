CC = gcc
CFLAGS = -O0

all: empty part1

empty: empty.c
	$(CC) $(CFLAGS) -o empty empty.c

part1: part1.c
	$(CC) $(CFLAGS) -o part1 part1.c

trace: all
	strace -o empty.trace ./empty
	strace -o part1.trace ./part1
	@wc -l empty.trace part1.trace

clean:
	rm -f empty part1

.PHONY: all trace clean
