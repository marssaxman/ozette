# ozette-specific settings
TARGET:=ozette
CCFLAGS:=-Werror -Wall -g
LDFLAGS:=-lpanel -lncurses -lpthread -lstdc++

# boilerplate rules
SOURCES:=$(shell find src -name *.c -o -name *.cpp)
OBJECTS:=$(addsuffix .o,$(basename $(patsubst src/%,bin/%,$(SOURCES))))
CCFLAGS+=-Isrc -MD -MP
$(TARGET): bin/$(TARGET)
default: $(TARGET)
bin/$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	g++ -o $@ $^ $(LDFLAGS)
bin/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(CC) -std=c++11 $(CCFLAGS) -c $< -o $@
bin/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) -std=c99 $(CCFLAGS) -c $< -o $@
clean:
	-rm -rf bin
install:
	cp bin/$(TARGET) /usr/bin/$(TARGET)
.PHONY: clean $(TARGET) install
-include $(shell find bin -name *.d)

# regenerate the help file
src/app/help.cpp: HELP
	xxd -i $^ $@
