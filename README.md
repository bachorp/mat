# The Multi Agent Transportation Problem - Supplement

![](teaser.gif)

## Test results and evaluation

For test data and evaluation please refer to the corresponding subfolders `input`, `configuration` and `mapf`.

## Proof of Lemma 2

The computational verification of Lemma 2 for basic graphs has been implemented using the algebra software `SageMath`.
The code can be found in the files `pebbles.sage` and `tzero.sage` and can be ran online at <https://sagecell.sagemath.org/>.
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

You can navigate in the found MAT plan by entering `f` (forward) `d` (back) and `c` (escape).

## Conflict-Based Search for MAPD

CBS-MAPD is a modified version of CBS-TA from [libMultiRobotPlanning](https://github.com/whoenig/libMultiRobotPlanning).

You can use Makefile to build from source.

```shell
make cbs
```

### Usage:

Since it was mainly build for comparison purposes, CBS-MAPD does not provide the same interface capabilities as MAT.

```shell
cbs_mapd [[OPTION value] ...]
```

where OPTION is either one of the following.

* `g[rid_size]` - Grid size (side length). Value must be a positive integer.
* `b[locked]` - Percentage of blocked grid cells. Value must be a non-negative between 0 and 100.
* `a[gents]` - Number of agents. Value must be an integer in range.
* `c[ontainers]` - Number of containers. Value must be an integer in range.
* `s[eed]` - Seed. Value can be any string, possibly empty.
* `o[utput]` - Output file name. Will generate an .yaml file with statistics and problem solution.

The instance generation works the same as for MAT,
if the same respective parameters are given the same instance will be generated.
Note, that when given more containers than agents, CBS-MAPD will always fail.