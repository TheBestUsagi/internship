CC=gcc
CFLAGS=-O0 -g -Wall -Wextra -pedantic

all: app_bug

app_bug: src/main_bug.c
	$(CC) $(CFLAGS) $^ -o $@

app_fix: src/main.c
	$(CC) $(CFLAGS) $^ -o $@

run-bug: app_bug
	./app_bug

run-fix: app_fix
	./app_fix

clean:
	rm -f app_bug app_fix
