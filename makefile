CC = gcc
CFLAGS = -Wall -std=c11

miasm: miasm.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm miasm
