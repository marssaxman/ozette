OUTFILE := "ozette"
CCFLAGS := -Wall -Wno-endif-labels -g
CCFLAGS += -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS
LDFLAGS := -lpanel -lncurses -lpthread -lstdc++


all: executable help

executable:
	@OUTFILE="$(OUTFILE)" CCFLAGS="$(CCFLAGS)" LDFLAGS="$(LDFLAGS)" \
	./build-exe.sh

clean:
	@rm -rf $(OUTFILE) ./obj/*

install:
	@./install.sh

help:
	@./build-help.sh











