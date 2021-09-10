P=dcs
OBJECTS=src/dcs.c src/dcs.h
EMCC=emcc
EMCC_CFLAGS=-s SIDE_MODULE=2 -c
BUILDDIR=build
DEPENDENCIES=
CP = cp

$(P): $(OBJECTS)
ifeq ($(OS), Windows_NT)
	cmd /C "copy src/dcs.h build/dcs.h"
else
	cp src/dcs.h build/dcs.h
endif
	$(EMCC) $(EMCC_CFLAGS) $(DEPENDENCIES) src/dcs.c -o $(BUILDDIR)/$(P).o
# cp build/dcs.o example/dependencies/dcs/dcs.o
# cp build/dcs.h example/dependencies/dcs/dcs.h
 