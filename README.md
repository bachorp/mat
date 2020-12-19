# The Multi Agent Transportation Problem - Supplement

![](teaser.gif)

## Test results and evaluation

For test data and evaluation please refer to the corresponding subfolders `input`, `configuration` and `mapf`.

## Proof of Lemma 2

The computational verification of Lemma 2 for basic graphs has been implemented using the algebra software `SageMath`.
The code can be found in the file `pebbles.sage` and can be ran online at <https://sagecell.sagemath.org/>.
## MAT solver

To install the MAT solver please get Cryptominisat 5 from <https://github.com/msoos/cryptominisat> and ensure that the header files and the dynamic library file are in appropriate paths.

You can use Makefile to build and install from source.

```shell
make [clean | build | install | uninstall]
```

### Usage:

```shell
mat [r[egular_mapf]] COMMAND [[OPTION value] ...] [outfile]
```

where COMMAND refers to either one of the following.

* `0` - Conduct a number of tests. Takes any of the possible options.
* `1` - Show the first example (Figure 1) from the paper.
* `2` - Show the counter example (Figure 2) from the paper.
* `3` - Show the counter example (Figure 3) from the paper.
* `b[arták]` - Conduct the test set from Barták et al.. Takes only a seed option.
* `i[nteractive]` - Start an interactive parameter prompt.

And OPTION is either one of the following.

* `g[rid_size]` - Grid size (side length). Value must be a positive integer.
* `s[eed]` - Seed. Value can be any string, possibly empty.
* `c[onfiguration]` - Configuration from a fixed set of configurations. Value must be an integer in range.

If an output file is given, results be written as comma seperated values (csv).

If no arguments are given, interactive mode will be started.
