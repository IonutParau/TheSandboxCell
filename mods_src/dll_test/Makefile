CC=gcc

LINKER=gcc

OUTPUT=mod.so

all:
	cp $(OUTPUT) ../../mods/dllmod/$(OUTPUT)
dll: main.o
	$(LINKER) -shared -o $(OUTPUT) main.o
main.o: main.c
	$(CC) -c -fPIC -o main.o -std=c23 main.c
