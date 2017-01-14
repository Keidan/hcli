SHELL=/bin/bash

help:
	@echo "make help: Print this help"
	@echo "make all: Compile all the sources"
	@echo "make clean: Clean the compiled files"

all:
	@mkdir -p deploy/$(shell uname -m)
	@cd src && $(MAKE) -f Makefile all

clean:
	@find . -name "*~" -type f -exec rm -f {} \;
	@cd src && $(MAKE) -f Makefile clean

