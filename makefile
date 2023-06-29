export CFLAGS+=-std=c18 -pedantic -Werror

export OUTDIR = out
export BOX_OUTDIR = ../$(OUTDIR)
export TOY_OUTDIR = ../../$(OUTDIR)

all: toy box game

toy: $(OUTDIR)
	$(MAKE) -j8 -C Box/Toy/source

box: $(OUTDIR)
	$(MAKE) -j8 -C Box/source

game: $(OUTDIR)
	$(MAKE) -j8 -C source
	ln -f -s ../assets -t $(OUTDIR)

#release
toy-release: $(OUTDIR)
	$(MAKE) -j8 -C Box/Toy/source library-release

box-release: $(OUTDIR)
	$(MAKE) -j8 -C Box/source library-release

game-release: $(OUTDIR)
	$(MAKE) -j8 -C source game-release
	cp -r assets $(OUTDIR)

#distribution
dist: clean
dist: export CFLAGS+=-O2 -mtune=native -march=native
dist: toy-release box-release game-release

$(OUTDIR):
	mkdir $(OUTDIR)

.PHONY: clean

clean:
ifeq ($(findstring CYGWIN, $(shell uname)),CYGWIN)
	find . -type f -name '*.o' -exec rm -f -r -v {} \;
	find . -type f -name '*.a' -exec rm -f -r -v {} \;
	find . -type f -name '*.exe' -exec rm -f -r -v {} \;
	find . -type f -name '*.dll' -exec rm -f -r -v {} \;
	find . -type f -name '*.lib' -exec rm -f -r -v {} \;
	find . -type f -name '*.so' -exec rm -f -r -v {} \;
	find . -empty -type d -delete
else ifeq ($(shell uname),Linux)
	find . -type f -name '*.o' -exec rm -f -r -v {} \;
	find . -type f -name '*.a' -exec rm -f -r -v {} \;
	find . -type f -name '*.exe' -exec rm -f -r -v {} \;
	find . -type f -name '*.dll' -exec rm -f -r -v {} \;
	find . -type f -name '*.lib' -exec rm -f -r -v {} \;
	find . -type f -name '*.so' -exec rm -f -r -v {} \;
	rm -rf out
	find . -empty -type d -delete
else ifeq ($(OS),Windows_NT)
	$(RM) *.o *.a *.exe 
else
	@echo "Deletion failed - what platform is this?"
endif

rebuild: clean all
