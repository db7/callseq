build:
	chicken-install -n

install:
	chicken-install

clean:
	chicken-install -purge
	rm -rf *.import.* *.link *.static.* *.build.sh *.install.sh *.so *.o

