CC=gcc

IDIR+=. ../Box/Toy/source ../Box/source
CFLAGS+=$(addprefix -I,$(IDIR)) -g -Wall -W -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable
LIBS+=-lSDL2 -ltoy -lbox -lSDL2_image -lSDL2_ttf -lSDL2main -lSDL2

ODIR = obj
SRC = $(wildcard *.c)
OBJ = $(addprefix $(ODIR)/,$(SRC:.c=.o))

OUT=../$(OUTDIR)/airport

all: $(OBJ)
	$(CC) $(CFLAGS) -o $(OUT) $(OBJ) -Wl,-rpath,. -L../$(OUTDIR) $(LIBS)

$(OBJ): | $(ODIR)

$(ODIR):
	mkdir $(ODIR)

$(ODIR)/%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean

clean:
	$(RM) $(ODIR)
