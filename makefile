CC = gcc
CFLAGS = -Wall -std=c11

matcher: matcher.c
	$(CC) $(CFLAGS) $< -o $@

miasm: miasm.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm miasm matcher
