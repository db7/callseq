CSCFLAGS+=	-C -D_XOPEN_SOURCE=700
CSCFLAGS+=	-C -Wno-deprecated-declarations
CSCFLAGS+=	-I../vendor/libvsync/include
CSCFLAGS+=	-I../include
CSCFLAGS+=	-O3 -e

all: bench-plain bench-ucontext bench-assembly bench-cncb bench-callseq

%.c: %.scm
	csc -t $(CSCFLAGS) -o $@ $^

bench-plain: bench.c plain.c
	csc $(CSCFLAGS) -o $@ $^

bench-ucontext: bench.c indirect.c bar.c
	csc -C -DCROUTON_UCONTEXT $(CSCFLAGS) -o $@ $^

bench-assembly: bench.c indirect.c bar.c
	csc $(CSCFLAGS) -o $@ $^

bench-callseq: bench.c with-callseq.c
	csc -C -DCALLSEQ $(CSCFLAGS) -o $@ $^ -link callseq

bench-cncb: bench.c with-cncb.c
	csc -C -DCNCB $(CSCFLAGS) -o $@ $^

clean:
	rm -rf *.o bench-*
	rm -f with-*.c bar.c plain.c
