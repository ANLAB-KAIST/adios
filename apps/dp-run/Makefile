.PHONY: module
module: dp-run
 
arch = x64
ifndef mode
	mode = release
endif

src = $(shell readlink -f ../..)

CFLAGS = -O2
LDFLAGS = -ldl

dp-run: dp-run.c
	$(CC) -o $@ $(CFLAGS) dp-run.c $(LDFLAGS)

clean:
	rm -f dp-run
