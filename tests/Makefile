SRCS=	$(wildcard *.scm)
BINS=	$(SRCS:.scm=.bin)

.SECONDARY:
.PHONY: all clean

all: $(BINS)

%.c: %.scm
	csc -et -o $@ $^

%.bin: %.c
	csc -e -o $@ $^ -link callseq

clean:
	rm -rf *.bin *.c
