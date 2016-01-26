default: ozette
ozette: bin/ozette

include srctree.mk
-include $(call findtype, d, obj)

CPPFLAGS=-MD -MP -Wall -Wno-endif-labels -g -Werror
CPPFLAGS+=-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS
CXXFLAGS=-std=c++11
CFLAGS=-std=c99
LDLIBS=-lpanel -lncurses -lpthread -lstdc++
SRCEXTS=c cpp

bin/ozette: $(call objs, $(SRCEXTS), src, obj)
	@mkdir -p $(@D)
	g++ $^ $(LDLIBS) -o $@
obj/%.o: src/%.cpp
	mkdir -p $(@D)
	$(CC) -Isrc $(CXXFLAGS) -c $< -o $@
obj/%.o: src/%.c
	mkdir -p $(@D)
	$(CC) -Isrc $(CFLAGS) -c $< -o $@
src/app/help.cpp: HELP
	xxd -i HELP src/app/help.cpp

clean:
	@rm -rf bin obj

install: bin/ozette
	cp bin/ozette /usr/bin/

.PHONY: clean install









