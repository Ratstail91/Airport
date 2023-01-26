CC=gcc

IDIR+=. ../Toy/source ../box
CFLAGS+=$(addprefix -I,$(IDIR)) -g -Wall -W -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable
LIBS+=-lSDL2 -ltoy -lbox

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
