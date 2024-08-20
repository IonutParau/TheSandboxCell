CC=gcc
CFLAGS=-std=c17

LINKER=gcc
LFLAGS=

OUTPUT=mod.so

ifeq ($(MODE),DEBUG)
	CFLAGS += -Og
endif

ifeq ($(MODE),RELEASE)
	CFLAGS += -O3 -flto
	LFLAGS += -O3
endif

objects=main.o replicators.o

all: libtsc dll
	mkdir -p ../../mods/more_gens
	cp $(OUTPUT) ../../mods/more_gens/$(OUTPUT)
	cp -r textures ../../mods/more_gens
	cp config.json ../../mods/more_gens
libtsc:
	cd ../..; lua ./libtsc.lua generate > mods_src/more_gens/src/libtsc.h
dll: $(objects)
	$(LINKER) -shared -o $(OUTPUT) $(objects) $(LFLAGS)
clean:
	rm -rf ../../mods/more_gens $(objects)
main.o: src/main.c
	$(CC) -c -fPIC -o main.o src/main.c $(CFLAGS)
replicators.o: src/replicators.c
	$(CC) -c -fPIC -o replicators.o src/replicators.c $(CFLAGS)
