
installdir 	:= ../deploy/$(shell uname -m)
appname		:= hcli.elf

CXX		:= g++
FLAGS 		:=-fstack-protector-all -D_FORTIFY_SOURCE=2 -ffunction-sections -Wall -std=c++11
DEBUG_FLAGS 	:= -g -O0
# Compiler
CXXFLAGS 	:= $(DEBUG_FLAGS) $(FLAGS)
# Linker
LDFLAGS 	:= -lssl -lcrypto

srcfiles	:= $(shell find . -name "*.cpp" -type f)
objfiles	:= $(patsubst %.cpp, %.o, $(srcfiles))

.PHONY : bin clean all $(appname)

all: $(appname)

$(appname): $(objfiles)
	$(CXX) -o $(appname) $(objfiles) $(LDFLAGS)
	@mkdir -p $(installdir) 
	mv $(appname) $(installdir)

depend: .depend

.depend: $(srcfiles)
	rm -f ./.depend
	$(CXX) $(CXXFLAGS) -MM $^>>./.depend;

clean:
	rm -f *~ .depend *.o

include .depend
