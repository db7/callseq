
all: example

example: example.scm runner.c
	csc -e $^ -o $@ -link callseq

clean:
	rm -rf *.o example.c *.so example
