default: ozette

include srctree.mk
-include $(call findtype, d, obj)

FLAGS:=-MD -MP -Wall -Wno-endif-labels -g -Werror -Isrc
CXXFLAGS:=$(FLAGS) -std=c++11
CFLAGS:=$(FLAGS) -std=c99

LDLIBS=-lpanel -lncurses -lpthread -lstdc++

ozette: $(call cxx_objs, src, obj)
	@mkdir -p $(@D)
	g++ $^ $(LDLIBS) -o $@

obj/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(CC) $(CXXFLAGS) -c $< -o $@

obj/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

obj:
	mkdir -p $@

src/app/help.cpp: HELP
	xxd -i HELP src/app/help.cpp

clean:
	@rm -rf bin obj

install: ozette
	cp ozette /usr/bin/

.PHONY: clean install









