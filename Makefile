# make is not worth the hassle, so we'll do everything inside bash scripts.
# it has several annoying habits. one of them is that if it happens to find a
# directory named "obj", it will cd into that directory before making anything.
# another is that if there happens to be a directory with the name of a target,
# make will refuse to build that target, claiming that it is up to date. grrr.

all: ozette-executable

ozette-executable:
	@./build.sh ozette ./obj ./src

clean:
	@rm -rf ozette ./obj/*

install:
	@./install.sh













