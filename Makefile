CC=gcc
CFLAGS=-c -fPIC
# Extra C Flags (for cross-compiling)
ECFLAGS=

LINKER=gcc
LFLAGS=-lm -lpthread -ldl
# Extra linking flags (for cross-compiling)
ELFLAGS=

OUTPUT=thesandboxcell
LIBRARY=libtsc.so

objects=workers.o utils.o cell.o grid.o resources.o rendering.o subticks.o saving.o saving_buffer.o ui.o api.o tinycthread.o ticking.o modloader.o value.o

LINKRAYLIB=-lraylib -lGL -lpthread -ldl -lrt -lX11 -lm

ifdef OPENMP
	CFLAGS += -DTSC_USE_OPENMP -fopenmp
	LFLAGS += -l$(OPENMP)
endif

ifeq ($(MODE), TURBO)
	CFLAGS += -DTSC_TURBO -Ofast
	ifeq ($(CC), gcc)
		CFLAGS += -flto=auto
	endif
	LFLAGS += -Ofast
endif

ifeq ($(MODE), RELEASE)
	CFLAGS += -O3
	ifeq ($(CC), gcc)
		CFLAGS += -flto=auto
	endif
	# No LTO with clang, because it keeps causing linking issues
	#ifeq ($(CC), clang)
	#	CFLAGS += -flto=thin
	#endif
	LFLAGS += -O3
endif

ifeq ($(MODE), DEBUG)
	CFLAGS += -g3
	# The linker might give us some more info idk
	LFLAGS += -g3
endif

ifeq ($(FORCE_SINGLE_THREAD), 1)
	CFLAGS += -DTSC_SINGLE_THREAD
endif

ifeq ($(PROFILING), 1)
	CFLAGS += -pg
	LFLAGS += -pg
endif

ifeq ($(MARCH_NATIVE), 1)
	CFLAGS += -march=native
endif

# Features that can be removed for performance or size

ifdef SUPPORT_LUA;
	CFLAGS += -DTSC_SUPPORT_LUA
endif

ifdef JIT_LUA;
	CFLAGS += -DTSC_JIT_LUA
endif

ifdef SUPPORT_PYTHON;
	CFLAGS += -DTSC_SUPPORT_PYTHON
endif

ifdef NOGIL_PYTHON;
	CFLAGS += -DTSC_NOGIL_PYTHON
endif

ifdef SUPPORT_WASM;
	CFLAGS += -DTSC_SUPPORT_WASM
endif

CFLAGS += $(ECFLAGS)
LFLAGS += $(ELFLAGS)

tests=test_saving.o

all: library main.o
ifeq ($(MODE), TURBO)
	$(LINKER) -o $(OUTPUT) main.o $(objects) $(LINKRAYLIB) $(LFLAGS)
else
	$(LINKER) -o $(OUTPUT) main.o -L. -l:./$(LIBRARY) $(LINKRAYLIB) $(LFLAGS)
endif
clean:
	rm -f $(objects) $(LIBRARY) $(OUTPUT) $(tests) main.o testing.o test_$(OUTPUT)
test: library $(tests) testing.o
	$(LINKER) -o test_$(OUTPUT) $(tests) testing.o -L. -l:./$(LIBRARY) $(LINKRAYLIB) $(LFLAGS)
fresh: clean all
	
library: $(objects)
	$(LINKER) -o $(LIBRARY) -shared $(objects) $(LFLAGS)
main.o: src/main.c
	$(CC) $(CFLAGS) src/main.c -o main.o
testing.o: src/testing.c
	$(CC) $(CFLAGS) src/testing.c -o testing.o
test_saving.o: src/saving/test_saving.c
	$(CC) $(CFLAGS) src/saving/test_saving.c -o test_saving.o
workers.o: src/threads/workers.c
	$(CC) $(CFLAGS) src/threads/workers.c -o workers.o
utils.o: src/utils.c
	$(CC) $(CFLAGS) src/utils.c -o utils.o
cell.o: src/cells/cell.c
	$(CC) $(CFLAGS) src/cells/cell.c -o cell.o
grid.o: src/cells/grid.c
	$(CC) $(CFLAGS) src/cells/grid.c -o grid.o
ticking.o: src/cells/ticking.c
	$(CC) $(CFLAGS) src/cells/ticking.c -o ticking.o
resources.o: src/graphics/resources.c
	$(CC) $(CFLAGS) src/graphics/resources.c -o resources.o
rendering.o: src/graphics/rendering.c
	$(CC) $(CFLAGS) src/graphics/rendering.c -o rendering.o
subticks.o: src/cells/subticks.c
	$(CC) $(CFLAGS) src/cells/subticks.c -o subticks.o
saving.o: src/saving/saving.c
	$(CC) $(CFLAGS) src/saving/saving.c -o saving.o
saving_buffer.o: src/saving/saving_buffer.c
	$(CC) $(CFLAGS) src/saving/saving_buffer.c -o saving_buffer.o
ui.o: src/graphics/ui.c
	$(CC) $(CFLAGS) src/graphics/ui.c -o ui.o
api.o: src/api/api.c
	$(CC) $(CFLAGS) src/api/api.c -o api.o
modloader.o: src/api/modloader.c
	$(CC) $(CFLAGS) src/api/modloader.c -o modloader.o
value.o: src/api/value.c
	$(CC) $(CFLAGS) src/api/value.c -o value.o
tinycthread.o: src/threads/tinycthread.c
	$(CC) $(CFLAGS) src/threads/tinycthread.c -o tinycthread.o
