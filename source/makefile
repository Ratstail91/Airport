CC=gcc

IDIR+=. ../Toy/source ../core
CFLAGS+=$(addprefix -I,$(IDIR)) -DSDL_MAIN_HANDLED -g -Wall -W -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable
LIBS+=-lSDL2 -ltoy -lcore

ODIR = obj
SRC = $(wildcard *.c)
OBJ = $(addprefix $(ODIR)/,$(SRC:.c=.o))

OUT=../$(OUTDIR)/airport

all: $(OBJ)
	$(CC) $(CFLAGS) -o $(OUT) $(OBJ) -Wl,-rpath,. -L../$(LIBDIR) $(LIBS)

$(OBJ): | $(ODIR)

$(ODIR):
	mkdir $(ODIR)

$(ODIR)/%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean

clean:
	$(RM) $(ODIR)
