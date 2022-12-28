# The Multi Agent Transportation Problem - Supplement

![](teaser.gif)

## Test results and evaluation

For test data and evaluation please refer to the corresponding subfolders `eval/input`, `eval/configuration` and `eval/mapf`.

## Sagemath Scripts

For our theoretical analysis we utilized the algebra software `SageMath`.
Code can be found in folder `sage` and can be ran online at <https://sagecell.sagemath.org/>.

## MAT solver

You can use *Make* to build from source.

```shell
make mat
```

Requires [*Cryptominisat 5*](<https://github.com/msoos/cryptominisat>).

**Usage**

```shell
build/mat [r[egular_mapf]] COMMAND [[OPTION value] ...] [outfile]
```

where `COMMAND` refers to either one of the following.

* `0` - Conduct a number of tests. Takes any of the possible options
* `1` - Show the first example (Figure 1) from the paper
* `2` - Show the counter example (Figure 2) from the paper
* `3` - Show the counter example (Figure 3) from the paper
* `b[arták]` - Conduct the test set from `[1]`. Takes only a seed option
* `i[nteractive]` - Start an interactive parameter prompt
* `n[on_blocking]` - Variant with non-blocking containers
* `f[ixed]` - Variant where a container can be transported by at most one agent

And `OPTION` is either one of the following.

* `g[rid_size]` - Grid size (side length): Value must be a positive integer
* `s[eed]` - Seed: Value can be any string, possibly empty
* `c[onfiguration]` - Configuration from a fixed set of configurations: Value must be an integer in range

If an output file is given, results be written as comma seperated values (`.csv`).

If no arguments are given, interactive mode will be started.

You can navigate in the found MAT plan by entering `f` (forward) `d` (back) and `c` (escape).

## Conflict-Based Search for MAPD

CBS-MAPD is a modified version of CBS-TA from [libMultiRobotPlanning](https://github.com/whoenig/libMultiRobotPlanning).

You can use *Make* to build from source.

```shell
make cbs_mapd
```

Requires [*Boost*](https://www.boost.org/).

**Usage**

Since it was mainly build for comparison purposes, CBS-MAPD does not provide the same interface capabilities as MAT.

```shell
build/cbs_mapd [[OPTION value] ...]
```

where `OPTION` is either one of the following.

* `g[rid_size]` - Grid size (side length): Value must be a positive integer
* `b[locked]` - Percentage of blocked grid cells: Value must be a non-negative between 0 and 100
* `a[gents]` - Number of agents: Value must be an integer in range
* `c[ontainers]` - Number of containers: Value must be an integer in range
* `s[eed]` - Seed: Value can be any string, possibly empty
* `o[utput]` - Output file name: Will generate an .yaml file with statistics and problem solution

The instance generation is the same as for MAT, i.e. if the same respective parameters are given the same instance will be generated.
Note that when given more containers than agents, CBS-MAPD will always fail.

---

`[1]` *Barták, R; Zhou, N; Stern, R; Boyarski, E; and Surynek, P. 2017. Modeling and Solving the Multi-agent Pathfinding Problem in Picat. In 29th IEEE International Conference on Tools with Artificial Intelligence, ICTAI 2017, 959-966. IEEE Computer Society.*
