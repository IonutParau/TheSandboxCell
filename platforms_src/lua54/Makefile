CC=gcc
CFLAGS=-std=c17

LINKER=gcc
LFLAGS=
LUADLL=liblua.a
LINKLUA=-L. -llua

OUTPUT=platform.so

ifeq ($(MODE),DEBUG)
	CFLAGS += -Og
endif

ifeq ($(MODE),RELEASE)
	CFLAGS += -O3 -flto
	LFLAGS += -O3
endif

objects=main.o common.o api.o

LFLAGS += $(LINKLUA)

all: libtsc dll
	mkdir -p ../../platforms/lua54
	cp $(OUTPUT) ../../platforms/lua54/$(OUTPUT)
	cp $(LUADLL) ../../platforms/lua54/$(LUADLL)
libtsc:
	cd ../..; lua ./libtsc.lua generate > platforms_src/lua54/src/libtsc.h
dll: $(objects)
	$(LINKER) -shared -o $(OUTPUT) $(objects) $(LFLAGS)
clean:
	rm -rf ../../platforms/lua54 $(objects)
main.o: src/main.c
	$(CC) -c -fPIC -o main.o src/main.c $(CFLAGS)
common.o: src/common.c
	$(CC) -c -fPIC -o common.o src/common.c $(CFLAGS)
api.o: src/api/api.c
	$(CC) -c -fPIC -o api.o src/api/api.c $(CFLAGS)
