CC=gcc

IDIR+=.
CFLAGS+=$(addprefix -I,$(IDIR)) -DSDL_MAIN_HANDLED -g -Wall -W -pedantic -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable
LIBS+=-lSDL2

ODIR = obj
SRC = $(wildcard *.c)
OBJ = $(addprefix $(ODIR)/,$(SRC:.c=.o))

OUTNAME=core

ifeq ($(findstring CYGWIN, $(shell uname)),CYGWIN)
	LIBLINE =-Wl,--out-implib=$(CORE_OUTDIR)/lib$(OUTNAME).dll.a -Wl,--export-all-symbols -Wl,--enable-auto-import -Wl,--whole-archive $(OBJ) -Wl,--no-whole-archive
	OUT=$(CORE_OUTDIR)/$(OUTNAME).dll
else ifeq ($(shell uname),Linux)
# No linux for the time being
else ifeq ($(OS),Windows_NT)
	LIBLINE =-Wl,--out-implib=$(CORE_OUTDIR)/lib$(OUTNAME).dll.a -Wl,--export-all-symbols -Wl,--enable-auto-import -Wl,--whole-archive $(OBJ) -Wl,--no-whole-archive
	OUT=$(CORE_OUTDIR)/$(OUTNAME).dll
else
	@echo "Platform test failed - what platform is this?"
	exit 1
endif

library: $(OBJ)
	$(CC) -DCORE_EXPORT $(CFLAGS) -shared -o $(OUT) $(LIBLINE) $(LIBS)

static: $(OBJ)
	ar crs $(CORE_OUTDIR)/lib$(OUTNAME).a $(OBJ) $(LIBS)

$(OBJ): | $(ODIR)

$(ODIR):
	mkdir $(ODIR)

$(ODIR)/%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean

clean:
	$(RM) $(ODIR)
