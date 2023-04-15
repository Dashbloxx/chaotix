CC := gcc
CFLAGS := -I..

SRCS=$(wildcard *.c)
BINS=$(patsubst %.c,%,$(SRCS))

all: $(BINS)

%: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(BINS)