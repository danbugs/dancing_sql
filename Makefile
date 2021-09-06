# NOTE: This Makefile was written for POSIX-compliant systems

P=dcs
OBJECTS=src/dcs.c src/dcs.h
EMCC=emcc
EMCC_CFLAGS=-s SIDE_MODULE=2 -c
BUILDDIR=build
DEPENDENCIES=

$(P): $(OBJECTS)
	cp src/dcs.h build/dcs.h
	$(EMCC) $(EMCC_CFLAGS) $(DEPENDENCIES) src/dcs.c -o $(BUILDDIR)/$(P).o
