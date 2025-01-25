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

objects=main.o common.o api.o cell_api.o category_api.o

LFLAGS += $(LINKLUA)

all: libtsc dll
	mkdir -p ../../data/platforms/lua54
	cp $(OUTPUT) ../../data/platforms/lua54/$(OUTPUT)
	cp $(LUADLL) ../../data/platforms/lua54/$(LUADLL)
libtsc:
	cd ../..; lua ./libtsc.lua generate > platforms/lua54/src/libtsc.h
dll: $(objects)
	$(LINKER) -shared -o $(OUTPUT) $(objects) $(LFLAGS)
clean:
	rm -rf ../../data/platforms/lua54 $(objects)
main.o: src/main.c
	$(CC) -c -fPIC -o main.o src/main.c $(CFLAGS)
common.o: src/common.c
	$(CC) -c -fPIC -o common.o src/common.c $(CFLAGS)
api.o: src/api/api.c
	$(CC) -c -fPIC -o api.o src/api/api.c $(CFLAGS)
cell_api.o: src/api/cell_api.c
	$(CC) -c -fPIC -o cell_api.o src/api/cell_api.c $(CFLAGS)
category_api.o: src/api/category_api.c
	$(CC) -c -fPIC -o category_api.o src/api/category_api.c $(CFLAGS)
