CC=gcc

IDIR+=. ../Toy/source
CFLAGS+=$(addprefix -I,$(IDIR)) -DSDL_MAIN_HANDLED -g -Wall -W -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable
LIBS+=-lSDL2 -lSDL2_image -ltoy

ODIR = obj
SRC = $(wildcard *.c)
OBJ = $(addprefix $(ODIR)/,$(SRC:.c=.o))

OUTNAME=core

ifeq ($(findstring CYGWIN, $(shell uname)),CYGWIN)
	LIBLINE =-Wl,--out-implib=$(CORE_OUTDIR)/lib$(OUTNAME).dll.a -Wl,--export-all-symbols -Wl,--enable-auto-import -Wl,--whole-archive $(OBJ) -Wl,--no-whole-archive
	OUT=$(CORE_OUTDIR)/$(OUTNAME).dll
else ifeq ($(shell uname),Linux)
	LIBLINE=-Wl,--out-implib=./$(CORE_OUTDIR)/lib$(OUTNAME).a -Wl,--whole-archive $(OBJ) -Wl,--no-whole-archive
	OUT=./$(CORE_OUTDIR)/lib$(OUTNAME).so
	CFLAGS += -fPIC
else ifeq ($(OS),Windows_NT)
	LIBLINE =-Wl,--out-implib=$(CORE_OUTDIR)/lib$(OUTNAME).dll.a -Wl,--export-all-symbols -Wl,--enable-auto-import -Wl,--whole-archive $(OBJ) -Wl,--no-whole-archive
	OUT=$(CORE_OUTDIR)/$(OUTNAME).dll
else
	@echo "Platform test failed - what platform is this?"
	exit 1
endif

library: libs $(OBJ)
	$(CC) -DCORE_EXPORT $(CFLAGS) -shared -o $(OUT) $(LIBLINE) -L../$(LIBDIR) $(LIBS)

static: libs $(OBJ)
	ar crs $(CORE_OUTDIR)/lib$(OUTNAME).a $(OBJ) -L../$(LIBDIR) $(LIBS)

libs:
	cp ../Toy/repl/lib* .
	cp ../Toy/repl/repl_tools.* .

$(OBJ): | $(ODIR)

$(ODIR):
	mkdir $(ODIR)

$(ODIR)/%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean

clean:
	$(RM) $(ODIR)
