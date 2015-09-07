OUTFILE := "ozette"
OBJDIR := "./obj"
SRCDIR := "./src"
CCFLAGS := -Wall -Wno-endif-labels -g -falign-functions=4
CCFLAGS += -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS
CPP := "gcc"
LDFLAGS := -lpanel -lncurses -lpthread -lstdc++


all: build

build:
	@OUTFILE="$(OUTFILE)" \
	OBJDIR="$(OBJDIR)" \
	SRCDIR="$(SRCDIR)" \
	CCFLAGS="$(CCFLAGS)" \
	LDFLAGS="$(LDFLAGS)" \
	./build.sh

clean:
	@rm -rf $(OUTFILE) $(OBJDIR)/*

install:
	@./install.sh













