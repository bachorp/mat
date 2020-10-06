# MAT Paper

<https://github.com/msoos/cryptominisat>

```shell
g++ -std=c++17 -Wall -Wextra -pedantic -O -o mat -Iinclude -Llib src/test.cpp -lcryptominisat5
```

## Usage:

```shell
mat [r[egular_mapf]] COMMAND [[OPTION value] ...] [outfile]
```

where COMMAND refers to either one of the following.

* `0` - Conduct a number of tests. Takes any of the possible options.
* `1` - Show the example (Figure 1) from the paper.
* `2` - Show the counter example (Figure 2) from the paper.
* `b[arták]` - Conduct the test set from Barták et al.. Takes only a seed option.
* `i[nteractive]` - Start an interactive parameter prompt.

And OPTION is either one of the following.

* `g[rid_size]` - Grid size (side length). Value must be a positive integer.
* `s[eed]` - Seed. Value can be any string, possibly empty.
* `c[onfiguration]` - Configuration from a fixed set of configurations. Value must be an integer in range.

If an output file is given, results be written as comma seperated values (csv).

If no arguments are given, interactive mode will be started.
