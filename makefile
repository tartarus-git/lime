COMPILER := g++
# doesn't work with clang up there, compiler bug probably, REPORT!!! TODO

.PHONY: all build header partial_test clean

all: build

build: header partial_test 

header: bin/lime_build.o

bin/lime_build.o: lime_build.h bin/.dirstamp
	$(COMPILER) --std=c++20 -Wall -c lime_build.h -o bin/lime_build.o

partial_test: bin/partial_test

bin/partial_test: bin/partial_test.o
	$(COMPILER) --std=c++20 -Wall bin/partial_test.o -o bin/partial_test

bin/partial_test.o: partial_test.cpp lime_build.h bin/.dirstamp
	$(COMPILER) --std=c++20 -Wall -c partial_test.cpp -o bin/partial_test.o

bin/.dirstamp:
	mkdir -p bin
	touch bin/.dirstamp

clean:
	git clean -fdx -e "*.swp"
