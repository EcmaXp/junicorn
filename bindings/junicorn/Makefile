.PHONY: gen_const clean jar all lib samples install

all: gen_const
	$(MAKE) -f Makefile.build all

lib:
	$(MAKE) -f Makefile.build lib

samples:
	$(MAKE) -f  Makefile.build samples

jar:
	$(MAKE) -f Makefile.build jar

install: lib jar
	$(MAKE) -f Makefile.build install

uninstall:
	$(MAKE) -f Makefile.build uninstall

gen_const:
	cd .. && python const_generator.py java

clean:
	rm -f junicorn/*.class
	rm -f samples/*.class
	rm -f *.so
	rm -f *.dylib
	rm -f *.dll
