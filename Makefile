all: sbkcheck

.PHONY: clean

clean:
	rm -rf sbkcheck sbkcheck.dSYM

sbkcheck: sbkcheck.c
	$(CC) -o $@ -Wall -Wextra -g -lusb-1.0 $^
