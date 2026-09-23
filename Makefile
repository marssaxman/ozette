# ozette-specific settings
EXECNAME:=ozette
CCFLAGS:=-Werror -Wall -g
LDFLAGS:=-lpanel -lncurses -lpthread -lstdc++

# boilerplate rules
SOURCES:=$(shell find src -name '*.c' -o -name '*.cpp')
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

# tests use the application modules with doctest's main in place of src/main.cpp
TEST_SOURCES:=$(shell find tests -name '*.c' -o -name '*.cpp')
TEST_OBJECTS:=$(addsuffix .o,$(basename $(patsubst tests/%,build/tests/%,$(TEST_SOURCES)))) \
	$(filter-out build/main.o,$(OBJECTS))
# Always relink so removed sources are also removed from the test executable.
test: $(TEST_OBJECTS)
	$(CXX) -o build/ozette-tests $^ $(LDFLAGS) $(TEST_LDFLAGS)
	./build/ozette-tests
build/tests/%.o: tests/%.cpp
	@mkdir -p $(@D)
	$(CXX) -std=c++11 $(CCFLAGS) -Itests/vendor -pthread -c $< -o $@
build/tests/%.o: tests/%.c
	@mkdir -p $(@D)
	$(CC) -std=c99 $(CCFLAGS) -Itests/vendor -c $< -o $@

clean:
	-rm -rf build
install:
	cp $(TARGET) /usr/bin/$(EXECNAME)
.PHONY: clean install test
-include $(OBJECTS:.o=.d) $(TEST_OBJECTS:.o=.d)

# regenerate the help file
src/help/text.cpp: HELP
	xxd -i $^ $@
