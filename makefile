export OUTDIR = out
export LIBDIR = lib
export TOY_OUTDIR = ../$(LIBDIR)
export CORE_OUTDIR = ../$(LIBDIR)

all: $(OUTDIR) $(LIBDIR) toy core
	$(MAKE) -j8 -C source
ifeq ($(findstring CYGWIN, $(shell uname)),CYGWIN)
	cp $(LIBDIR)/*.dll $(OUTDIR)
else ifeq ($(shell uname),Linux)
	cp $(LIBDIR)/*.so $(OUTDIR)
else ifeq ($(OS),Windows_NT)
	cp $(LIBDIR)/*.dll $(OUTDIR)
endif

test: clean $(OUTDIR) toy core
	$(MAKE) -C test

toy: $(LIBDIR)
	$(MAKE) -j8 -C Toy/source

core: $(LIBDIR)
	$(MAKE) -j8 -C core

$(OUTDIR):
	mkdir $(OUTDIR)

$(LIBDIR):
	mkdir $(LIBDIR)

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
