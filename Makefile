.PHONY: all
all: clean build install

.PHONY: build
build:
	mkdir -p build
	g++ -std=c++17 -Wall -Wextra -pedantic -O -o build/mat src/test.cpp -lcryptominisat5

.PHONY: install
install:
	install build/mat ~/.local/bin

.PHONY: clean
clean:
	rm -rf build

.PHONY: uninstall
uninstall:
	rm ~/.local/bin/mat
