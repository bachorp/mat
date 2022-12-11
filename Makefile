.PHONY: all
all: clean mat cbs

.PHONY: clean
clean:
	rm -rf build

.PHONY: mat
mat:
	mkdir -p build
	g++ -std=c++17 -Wall -Wextra -pedantic -O -o build/mat src/test.cpp -L./cryptominisat/build/lib/ -lcryptominisat5

.PHONY: cbs
cbs:
	mkdir -p build
	g++ -std=c++17 -Wall -Wextra -pedantic -O -o build/cbs_mapd src/cbs_ta/cbs_mapd.cpp
