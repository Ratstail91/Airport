export OUTDIR = out
export BOX_OUTDIR = ../$(OUTDIR)
export TOY_OUTDIR = ../../$(OUTDIR)

all: $(OUTDIR) toy box
	$(MAKE) -j8 -C source

toy: $(LIBDIR)
	$(MAKE) -j8 -C Box/Toy/source

box: $(LIBDIR)
	$(MAKE) -j8 -C Box/source

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
