# ozette-specific settings
EXECNAME:=ozette
CCFLAGS:=-Werror -Wall -g
LDFLAGS:=-lpanel -lncurses -lpthread -lstdc++

# boilerplate rules
SOURCES:=$(shell find src -name *.c -o -name *.cpp)
OBJECTS:=$(addsuffix .o,$(basename $(patsubst src/%,build/%,$(SOURCES))))
CCFLAGS+=-Isrc -MD -MP
TARGET:=build/$(EXECNAME)
default: $(TARGET)
$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CC) -o $@ $^ $(LDFLAGS)
build/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(CC) -std=c++11 $(CCFLAGS) -c $< -o $@
build/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) -std=c99 $(CCFLAGS) -c $< -o $@
clean:
	-rm -rf build
install:
	cp $(TARGET) /usr/bin/$(EXECNAME)
.PHONY: clean install
-include $(shell find build -name *.d)

# regenerate the help file
src/help/text.cpp: HELP
	xxd -i $^ $@
