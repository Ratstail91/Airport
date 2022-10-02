CC=gcc

IDIR+=. ../Toy/source ../core
CFLAGS+=$(addprefix -I,$(IDIR)) -g -Wall -W -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable
LIBS+=-lSDL2 -ltoy -lcore

ODIR = obj
TARGETS = $(wildcard ../core/*.c)
TESTS = $(wildcard *.c)
OBJ = $(addprefix $(ODIR)/,$(TARGETS:../core/%.c=%.o)) $(addprefix $(ODIR)/,$(TESTS:.c=.o))

.PRECIOUS:  $(TESTS:%.c=../$(OUTDIR)/%.exe)

all: $(OBJ) $(TESTS:%.c=../$(OUTDIR)/%.exe)

../$(OUTDIR)/%.exe: $(ODIR)/%.o
	@$(CC) -o $@ $< $(TARGETS:../core/%.c=$(ODIR)/%.o) $(CFLAGS) -L../$(LIBDIR) $(LIBS)
	cp ../$(LIBDIR)/*.dll ../$(OUTDIR)
ifeq ($(shell uname),Linux)
	valgrind --leak-check=full --track-origins=yes $@
else
	$@
endif

$(OBJ): | $(ODIR)

$(ODIR):
	mkdir $(ODIR)

$(ODIR)/%.o: %.c
	@$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR)/%.o: ../core/%.c
	@$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean

clean:
	$(RM) $(ODIR)
