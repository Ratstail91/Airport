CC=gcc

IDIR+=. ../Toy/source ../box
CFLAGS+=$(addprefix -I,$(IDIR)) -DSDL_MAIN_HANDLED -g -Wall -W -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable
LIBS+=-lSDL2 -lSDL2_image -ltoy -lbox

ODIR = obj
TARGETS = $(wildcard ../box/*.c)
TESTS = $(wildcard *.c)
OBJ = $(addprefix $(ODIR)/,$(TARGETS:../box/%.c=%.o)) $(addprefix $(ODIR)/,$(TESTS:.c=.o))

.PRECIOUS:  $(TESTS:%.c=../$(OUTDIR)/%.exe)

all: $(OBJ) $(TESTS:%.c=../$(OUTDIR)/%.exe)

../$(OUTDIR)/%.exe: $(ODIR)/%.o
ifeq ($(shell uname),Linux)
	@$(CC) -o $@ $< $(TARGETS:../box/%.c=$(ODIR)/%.o) $(CFLAGS) -Wl,-rpath,../out -L../$(LIBDIR) $(LIBS)
	cp ../$(LIBDIR)/*.so ../$(OUTDIR)
	valgrind --leak-check=full --track-origins=yes $@
else
	@$(CC) -o $@ $< $(TARGETS:../box/%.c=$(ODIR)/%.o) $(CFLAGS) -L../$(LIBDIR) $(LIBS)
	cp ../$(LIBDIR)/*.dll ../$(OUTDIR)
	$@
endif

$(OBJ): | $(ODIR)

$(ODIR):
	mkdir $(ODIR)

$(ODIR)/%.o: %.c
	@$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR)/%.o: ../box/%.c
	@$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean

clean:
	$(RM) $(ODIR)
