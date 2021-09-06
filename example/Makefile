P=example
OBJECTS=src/main.c
EMCC=emcc
EMCC_CFLAGS=-s MAIN_MODULE=1 -lnodefs.js -s EXIT_RUNTIME=1
BUILDDIR=build
DEPENDENCIES=dependencies/dcs/dcs.o  

$(P): $(OBJECTS)
	$(EMCC) $(EMCC_CFLAGS) $(DEPENDENCIES) src/main.c -o $(BUILDDIR)/$(P).js