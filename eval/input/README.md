This directory contains a set of test data used to evaluate the effect of input characteristics.
We generated 20 instances per parameter set (4 ≤ g ≤ 12, b ∈ {10, 20}, 1 ≤ a, c ≤ 10).
For the evaluation we included exactly 10 instances for each parameter set none of which was recognized as unsolvable.

***Makespan, Runtime, and Formula size depending on the number of agents or containers***

```shell
python3 -m evaluate > out.txt
```

To speed up consecutive evaluations, the data are serialized and stored in `*.p`-files. Remove them if data change.

***Comparing runtime, share of solved instances, and makespan for problem- or implementation variants***

```shell
python3 -m plotter mat MAT fixed MAT-fixed_agents
python3 -m plotter mat MAT non_blocking MAT-non_blocking
python3 -m plotter cbs CBS-MAPD mapd SAT-MAPD
python3 -m plotter mat MAT mapd MAPD
```
