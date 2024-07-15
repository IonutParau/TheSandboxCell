CC=gcc

LINKER=gcc

OUTPUT=mod.so

all: libtsc dll
	cp $(OUTPUT) ../../mods/dllmod/$(OUTPUT)
libtsc:
	cd ../..; lua ./libtsc.lua generate > mods_src/dll_test/libtsc.h
dll: main.o
	$(LINKER) -shared -o $(OUTPUT) main.o
main.o: main.c
	$(CC) -c -fPIC -o main.o -std=c23 main.c
