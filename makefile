COMPILER := g++
# doesn't work with clang up there, compiler bug probably, REPORT!!! TODO

.PHONY: all build header test clean

all: build

build: header test

header: bin/lime_build.o

bin/lime_build.o: lime_build.cpp bin/.dirstamp
	$(COMPILER) --std=c++20 -Wall -c lime_build.cpp -o bin/lime_build.o

test: bin/test/test

bin/test/test: bin/test/test.o
	$(COMPILER) --std=c++20 -Wall bin/test/test.o -o bin/test/test

bin/test/test.o: test/test.cpp bin/test/.dirstamp
	$(COMPILER) --std=c++20 -Wall -c test/test.cpp -o bin/test/test.o

bin/.dirstamp:
	mkdir -p bin
	touch bin/.dirstamp

bin/test/.dirstamp: bin/.dirstamp
	mkdir -p bin/test
	touch bin/test/.dirstamp

clean:
	git clean -fdx -e "*.swp"
