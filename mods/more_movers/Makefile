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

objects=main.o pullers.o pushers.o

all: libtsc dll
	mkdir -p ../../data/mods/more_movers
	cp $(OUTPUT) ../../data/mods/more_movers/$(OUTPUT)
	cp -r textures ../../data/mods/more_movers
	cp config.json ../../data/mods/more_movers
libtsc:
	cd ../..; lua ./libtsc.lua generate > mods/more_movers/src/libtsc.h
dll: $(objects)
	$(LINKER) -shared -o $(OUTPUT) $(objects) $(LFLAGS)
clean:
	rm -rf ../../data/mods/more_movers $(objects)
main.o: src/main.c
	$(CC) -c -fPIC -o main.o src/main.c $(CFLAGS)
pullers.o: src/pullers.c
	$(CC) -c -fPIC -o pullers.o src/pullers.c $(CFLAGS)
pushers.o: src/pushers.c
	$(CC) -c -fPIC -o pushers.o src/pushers.c $(CFLAGS)
